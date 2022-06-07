import sqlite3
import requests

def request_handler(request):
    if request["method"] == "GET":
        try:
            accels = str(request['values']['accels'])
            stored_accels = getalldata()
            A_sig = list(stored_accels[0][0][0].split(", "))
            E_sig = list(stored_accels[1][0][0].split(", "))
            I_sig = list(stored_accels[2][0][0].split(", "))
            O_sig = list(stored_accels[3][0][0].split(", "))
            U_sig = list(stored_accels[4][0][0].split(", "))
            Y_sig = list(stored_accels[5][0][0].split(", "))
            accel_sig = list(accels.split(", "))
            if len(A_sig)>10:
                A_sig = [float(x) for x in A_sig[0:len(A_sig)-1]]
            else:
                A_sig = [float(x) for x in A_sig]

            if len(E_sig)>10:
                E_sig = [float(x) for x in E_sig[0:len(E_sig)-1]]
            else:
                E_sig = [float(x) for x in E_sig]

            if len(I_sig)>10:
                I_sig = [float(x) for x in I_sig[0:len(I_sig)-1]]
            else:
                I_sig = [float(x) for x in I_sig]

            if len(O_sig)>10:
                O_sig = [float(x) for x in O_sig[0:len(O_sig)-1]]
            else:
                O_sig = [float(x) for x in O_sig]

            if len(U_sig)>10:
                U_sig = [float(x) for x in U_sig[0:len(U_sig)-1]]
            else:
                U_sig = [float(x) for x in U_sig]

            if len(Y_sig)>10:
                Y_sig = [float(x) for x in Y_sig[0:len(Y_sig)-1]]
            else:
                Y_sig = [float(x) for x in Y_sig]

            if len(accel_sig)>10:
                accel_sig = [float(x) for x in accel_sig[0:len(accel_sig)-1]]
            else:
                accel_sig = [float(x) for x in accel_sig]

            if len(accel_sig)<len(A_sig):
                A_sig = A_sig[0:len(accel_sig)]
                corrA = correlation(A_sig, accel_sig)
            else:
                corrA = correlation(A_sig, accel_sig[0:len(A_sig)])

            if len(accel_sig)<len(E_sig):
                E_sig = E_sig[0:len(accel_sig)]
                corrE = correlation(E_sig, accel_sig)
            else:
                corrE = correlation(E_sig, accel_sig[0:len(E_sig)])

            if len(accel_sig)<len(I_sig):
                I_sig = I_sig[0:len(accel_sig)]
                corrI = correlation(I_sig, accel_sig)
            else:
                corrI = correlation(I_sig, accel_sig[0:len(I_sig)])

            if len(accel_sig)<len(O_sig):
                O_sig = O_sig[0:len(accel_sig)]
                corrO = correlation(O_sig, accel_sig)
            else:
                corrO = correlation(O_sig, accel_sig[0:len(O_sig)])

            if len(accel_sig)<len(U_sig):
                U_sig = U_sig[0:len(accel_sig)]
                corrU = correlation(U_sig, accel_sig)
            else:
                corrU = correlation(U_sig, accel_sig[0:len(U_sig)])

            if len(accel_sig)<len(Y_sig):
                Y_sig = Y_sig[0:len(accel_sig)]
                corrY = correlation(Y_sig, accel_sig)
            else:
                corrY = correlation(Y_sig, accel_sig[0:len(Y_sig)])
            

            maxcoor = max(corrA, corrE, corrI, corrO, corrU, corrY)

            if maxcoor==corrA:
                return 'A'
            elif maxcoor==corrE:
                return 'E'
            elif maxcoor == corrI:
                return 'I'
            elif maxcoor == corrO:
                return 'O'
            elif maxcoor==corrU:
                return 'U'
            elif maxcoor == corrY:
                return 'Y'

        except Exception as e:
            return 'error: ' + str(e)

    elif request["method"]=="POST":
        #POST part
        try:
            accels = str(request['form']['accels'])
            letter = str(request['form']['letter'])
            create_database()
            x = getalldata()
            if len(x[0])==0:
                initialize()
            else:
                update(accels,letter)

            return 'success'

        except Exception as e:
            return 'error: ' + str(e)

gestures_db = '/var/jail/home/divnor80/gestures_db.db'

def create_database():
    conn = sqlite3.connect(gestures_db)  # connect to that database (will create if it doesn't already exist)
    c = conn.cursor()  # move cursor into database (allows us to execute commands)
    c.execute('''CREATE TABLE IF NOT EXISTS test_table (letter text, accels text);''') # run a CREATE TABLE command
    conn.commit() # commit commands (VERY IMPORTANT!!)
    conn.close() # close connection to database

def initialize():
    conn = sqlite3.connect(gestures_db)  # connect to that database (will create if it doesn't already exist)
    c = conn.cursor()  # move cursor into database (allows us to execute commands)
    c.execute('''INSERT INTO test_table VALUES (?,?);''', ("A", "1, 1, 1"))
    c.execute('''INSERT INTO test_table VALUES (?,?);''', ("E", "1, 1, 1"))
    c.execute('''INSERT INTO test_table VALUES (?,?);''', ("I", "1, 1, 1"))
    c.execute('''INSERT INTO test_table VALUES (?,?);''', ("O", "1, 1, 1"))
    c.execute('''INSERT INTO test_table VALUES (?,?);''', ("U", "1, 1, 1"))
    c.execute('''INSERT INTO test_table VALUES (?,?);''', ("Y", "1, 1, 1"))
    conn.commit() # commit commands (VERY IMPORTANT!!)
    conn.close() # close connection to database


def update(accels, letter):
    conn = sqlite3.connect(gestures_db)
    c = conn.cursor()
    c.execute('''UPDATE test_table SET accels = ? WHERE letter = ?''', (accels, letter))
    conn.commit() # commit commands
    conn.close() # close connection to database

def getinfo(letter):
    conn = sqlite3.connect(gestures_db)
    c = conn.cursor()
    accel_vec = c.execute('''SELECT accels FROM test_table WHERE letter = (?);''',(letter,)).fetchall()
    conn.commit() # commit commands
    conn.close() # close connection to database
    return accel_vec

def getalldata():
    conn = sqlite3.connect(gestures_db)
    c = conn.cursor()
    accelA = c.execute('''SELECT accels FROM test_table WHERE letter = (?);''',('A',)).fetchall()
    accelE = c.execute('''SELECT accels FROM test_table WHERE letter = (?);''',('E',)).fetchall()
    accelI = c.execute('''SELECT accels FROM test_table WHERE letter = (?);''',('I',)).fetchall()
    accelO = c.execute('''SELECT accels FROM test_table WHERE letter = (?);''',('O',)).fetchall()
    accelU = c.execute('''SELECT accels FROM test_table WHERE letter = (?);''',('U',)).fetchall()
    accelY = c.execute('''SELECT accels FROM test_table WHERE letter = (?);''',('Y',)).fetchall()
    conn.commit() # commit commands
    conn.close() # close connection to database
    return accelA, accelE, accelI, accelO, accelU, accelY

# def offset_and_normalize(inp):
#     s = 0
#     xbar = sum(inp)/len(inp)
#     for xi in inp:
#         s+=(xi-xbar)**2
#     s = s**0.5
#     out = [(xi-xbar)/s for xi in inp]
#     return out

def correlation(x,y):
    # nx = offset_and_normalize(x)
    # ny = offset_and_normalize(y)
    a = 0
    for i in range(len(x)):
        a+=(x[i]*y[i])
    return a