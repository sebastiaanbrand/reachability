import os

look_for = 'Model ,|S| ,Time ,Mem(kb) ,fin. SDD ,fin. DDD ,peak SDD ,peak DDD ,SDD Hom ,SDD cache peak ,DDD Hom ,DDD cachepeak ,SHom cache\n'

output_file = 'bench_data/its_tools/its_tools_petrinets.csv'

def write_header():
    with open(output_file, 'w') as f:
        f.write('benchmark, time, memory_kb, states\n')

def write_line(data):
    with open(output_file, 'a') as f:
        f.write('{}, {}, {}, {}\n'.format(*data))

def process_line(data_line):
    data = data_line.split(',')
    states = data[1]
    time = data[2]
    memory = data[3]
    return [0, time, memory, states]

def load_file(folder, filename):
    with open(folder + filename, 'r') as f:
        lines = f.readlines()
        for index, line in enumerate(lines):
            if (line == look_for):
                data = process_line(lines[index+1])
                data[0] = filename[:-4]
                write_line(data)
                

def process_data(folder):
    write_header()
    for filename in os.listdir(folder):
        load_file(folder, filename)


if __name__ == '__main__':
    process_data('bench_data/its_tools/petrinets/')