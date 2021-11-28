import random
import argparse
import os , sys
from shutil import copyfile
import re
import subprocess

def main(input_path , output_path , typeChange , inverter , rewire):
    try:
        os.mkdir(output_path)
    except:
        pass
    copyfile(input_path , output_path + '/g1.v')
    output = open(output_path + '/r2.v' , 'w')
    output.writelines("// Original file path : " + input_path + "\n")
    output.writelines("// Number of gate type changed : " + str(typeChange) + "\n")
    output.writelines("// Number of gate inverted : " + str(inverter) + "\n")
    output.writelines("// Number of rewired (pair) : " + str(rewire) + "\n")

    gateTypes = ["or" , "nor" , "and" , "nand" , "xor" , "xnor" , "not" , "buf"]
    with open(input_path, 'r', newline='') as file_in:
        f = file_in.read().splitlines()
        count = 0
        find = False
        for lines in f:
            striped = lines.strip()
            for gate in gateTypes:
                if striped.startswith(gate):
                    find = True
            if find:
                break
            count += 1
        if f[len(f) - 1].startswith("endmodule"):
            lastLine = len(f) - 2
        else:
            lastLine = len(f) - 3
        
        NOTorBUF = []
        otherGates = []
        for index , lines in enumerate(f[count : lastLine + 1]):
            striped = lines.strip()
            if striped.startswith("buf") or striped.startswith("not"):
                NOTorBUF.append(index + count)
            else:
                otherGates.append(index + count)
        if typeChange + inverter > lastLine + 1 - count:
            print("Number of gate type changes and inverters overflows!!!")
        if rewire * 2 > lastLine + 1 - count:
            print("Number of rewires overflows!!!")
        toRewire = random.sample(range(count , lastLine + 1) , rewire * 2)
        toChange = random.sample(otherGates , typeChange)
        for i in range(0 , len(toChange)):
            otherGates.remove(toChange[i])
        if len(otherGates) != 0:
            for i in range(0 , len(otherGates)):
                NOTorBUF.append(otherGates[i])
        toInvert = random.sample(NOTorBUF , inverter)
        temp1 = [x + 8 for x in toChange]
        temp2 = [x + 8 for x in toInvert]
        temp3 = [x + 8 for x in toRewire]
        output.writelines("// Lines that change type : " + str(temp1) + "\n")
        output.writelines("// Lines that are inverted : " + str(temp2) + "\n")
        output.writelines("// Lines that have been rewired : " + str(temp3) + "\n")
        Wires = []
        WireIndex = []
        for index in toRewire:
            lines = f[index]
            striped = lines.strip()
            lineArr = re.split('\(|\s\(|\s\s|\s,|\s|,\s|,|\)' , striped)
            wire = random.randint(3 , len(lineArr) - 2)
            WireIndex.append(wire)
            Wires.append(lineArr[wire])

        for index , lines in enumerate(f):
            striped = lines.strip()
            if index not in toChange and index not in toInvert and index not in toRewire:
                output.writelines(striped)
                output.writelines("\n")
                continue
            lineArr = re.split('\(|\s\(|\s\s|\s,|\s|,\s|,|\)' , striped)
            length = len(lineArr)
            gate = lineArr[0]
            if index in toChange:
                lineArr.append("   // Gate type changed ! Original gate type : " + gate)
                gateTypes2 = ["or" , "and" , "xor"]
                gateTypes3 = ["nor" , "nand" , "xnor"]
                if gate in gateTypes2:
                    gateTypes2.remove(gate)
                    lineArr[0] = random.sample(gateTypes2 , 1)[0]
                else:
                    gateTypes3.remove(gate)
                    lineArr[0] = random.sample(gateTypes3 , 1)[0]
            if index in toInvert:
                lineArr.append("   // Gate inverted !")
                gateType = gateTypes.index(gate)
                if gateType % 2 == 1:
                    gateType -= 1
                else:
                    gateType += 1
                lineArr[0] = gateTypes[gateType]

            if index in toRewire:
                inx = toRewire.index(index)
                lineArr.append("   // Rewire " + Wires[inx] + " with")
                if inx % 2 == 0:
                    another = Wires[inx + 1]
                    lineArr.append(another + " in line " + str(toRewire[inx + 1] + 8))
                else:
                    another = Wires[inx - 1]
                    lineArr.append(another + " in line " + str(toRewire[inx - 1] + 8))
                lineArr[WireIndex[inx]] = another

            output.writelines(lineArr[0] + " " + lineArr[1] + "(" + lineArr[2])
            for i in range(3 , length - 1):
                output.writelines(", " + lineArr[i])
            output.writelines(");")
            string = " "
            output.writelines(string.join(lineArr[length:]))
            output.writelines("\n")
    output.close()

    # create dofile
    dofile = open(".dofile", "w")
    dofile.write( "read_verilog " + output_path + '/r2.v' )
    dofile.close()

if __name__ == '__main__' :
    input_path = sys.argv[1] 
    output_path = sys.argv[2]
    typeChange = int(sys.argv[3])
    inverter = int(sys.argv[4])
    rewire = int(sys.argv[5])
    main(input_path, output_path , typeChange , inverter , rewire)

    subprocess.run(["./../abc/abc" , "-f" , ".dofile"])