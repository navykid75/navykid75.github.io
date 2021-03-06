import pickle

dbfilename = 'test3_4.dat'


# need to add functions

def readScoreDB():
    try:
        fH = open(dbfilename, 'rb')
    except FileNotFoundError as e:
        print("New DB : ", dbfilename)
        return []
    scdb = []
    try:
        scdb = pickle.load(fH)
    except:
        print("Empty DB : ", dbfilename)
    else:
        print("Open DB : ", dbfilename)
    fH.close()
    return scdb


# write the data into person db


def writeScoreDB(scdb):
    fH = open(dbfilename, 'wb')
    pickle.dump(scdb, fH)
    fH.close()


def doScoreDB(scdb):
    while (True):
        inputStr = (input("Score DB > "))

        if inputStr == "":
            continue

        parse = inputStr.split(" ")

        # add 명령 구현
        if parse[0] == 'add':
            try:
                record = {'Name': parse[1], 'Age': parse[2], 'Score': parse[3]}
                # add 명령어를 포함하여 4개의 데이터가 입력되지 않았을 때
                if len(parse) != 4:
                    raise Exception("add 명령을 수행하기 위해서 3개의 데이터가 필요합니다")
            except:
                print("다시입력 해주세요")
            else:
                scdb += [record]


        # del 명령 구현
        elif parse[0] == 'del':
            try:
                if len(parse) != 2:
                    raise IndexError  # del 명령을 수행하기 위한 변수의 개수가 충족되지 않았을 떄
            except IndexError:
                print("del 명령에 맞는 데이터 갯수를 지켜주세요")
            else:
                scdb = delPersonDB(scdb, parse[1])

        # find 명령 구현
        elif parse[0] == 'find':
            try:
                if len(parse) != 2:
                    raise IndexError
            except:
                print("find 명령에 맞는 데이터 갯수를 지켜주세요")
            else:
                scdb_1 = findPersonDB(scdb, parse[1])
                for p in scdb_1:
                    for attr in sorted(p):
                        print(attr + " = " + p[attr], end=' ')
                    print()



        elif parse[0] == 'inc':
            try:
                if len(parse) != 2:
                    raise IndexError
            except:
                print("inc 명령에 맞는 데이터 갯수를 지켜주세요")
            else:
                for p in scdb:
                    if p['Name'] == parse[1]:
                        p['Score'] = str(int(p['Score']) + int(parse[2]))

        elif parse[0] == 'show':
            sortKey = 'Name' if len(parse) == 1 else parse[1]
            showScoreDB(scdb, sortKey)

        elif parse[0] == 'quit':
            break

        else:
            print("invalid command" + parse[0])


def showScoreDB(scdb, keyname):
    for p in sorted(scdb, key=lambda person: person[keyname]):
        for attr in sorted(p):
            print(attr + " = " + p[attr], end=' ')
        print()


def findPersonDB(scdb, name):
    return [p for p in scdb if p['Name'] == name]


def delPersonDB(scdb, name):
    return [p for p in scdb if p['Name'] != name]


if __name__ == "__main__":
    scoredb = readScoreDB()
    doScoreDB(scoredb)
    writeScoreDB(scoredb)


