import os
import sys

look_for = 'Model ,|S| ,Time ,Mem(kb) ,fin. SDD ,fin. DDD ,peak SDD ,peak DDD ,SDD Hom ,SDD cache peak ,DDD Hom ,DDD cachepeak ,SHom cache\n'

output_file = '{}its_tools_petrinets.csv'

def write_header():
    with open(output_file, 'w') as f:
        f.write('benchmark, type, reach_time, memory_kb, final_states\n')

def write_line(data):
    with open(output_file, 'a') as f:
        f.write('{}, {}, {}, {}, {}\n'.format(*data))

def process_line(data_line):
    data = data_line.split(',')
    states = data[1]
    time = data[2]
    memory = data[3]
    return [0, 0, time, memory, states]

def load_file(folder, filename):
    #print("processing {}{}".format(folder, filename))
    with open(folder + filename, 'r') as f:
        lines = f.readlines()
        for index, line in enumerate(lines):
            if (line == look_for):
                data = process_line(lines[index+1])
                bench_name = filename[:-4]
                if (bench_name[-13:] == '.pnml.img.gal'):
                    data[0] = bench_name[:-13]
                    data[1] = '.img.gal'
                elif (bench_name[-9:] == '.pnml.gal'):
                    data[0] = bench_name[:-9]
                    data[1] = '.gal'
                else:
                    print("unexpected file extension in benchmark name:")
                    print(bench_name)
                    exit(1)
                write_line(data)
                

def process_data(folder):
    write_header()
    print("Aggregating individual output files...")
    for filename in sorted(os.listdir(folder)):
        load_file(folder, filename)
    print(f"Written aggregated results to {output_file}")


if __name__ == '__main__':

    if (len(sys.argv) <= 1):
        print("Please the bench data folder which contains a folder its_tools/ with the output logs (possibly 'bench_data/reach-vs-its/')")
        exit(1)
    data_folder = sys.argv[1]
    output_file = output_file.format(data_folder)
    process_data(f'{data_folder}its_tools/')