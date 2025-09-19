import subprocess

open("Outputs/outputs.txt", "w").close()

lines = 0

output = open("Outputs/outputs.txt", "a")
for k in range(2):

    size = 10

    for i in range (1,11):
        subprocess.run(
            "make clean -C Greedy", shell=True,
            capture_output=True
        )

        subprocess.run(
            "make K=" + str(i * size) + " R=" + str(int(i*size/4)) + " RANDOMS=" + str(int(i)*k) + " -C Greedy/", shell=True,
            capture_output=True 
        )


        for best in range(2):
            for j in range(int(i*size/4) + 1):
                result = subprocess.run(
                    ["./Inputs/generator", str(0), str(best), str(j)],
                    capture_output=True,
                    text=True
                )

                with open("Inputs/inputs_no_predictions.txt", "w") as input:
                    input.write(result.stdout)

                result = subprocess.run(["python3", "no_predictions.py"], capture_output=True, text=True)

                output.write(result.stdout)

                
                lines = lines + 1

                print(lines)
        
output.close()