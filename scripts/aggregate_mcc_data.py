import os

look_for = "##teamcity[testFinished name='all' duration='"

output_file = 'bench_data/its_tools/its_tools_petrinets_deadlocks.csv'

def write_header():
    with open(output_file, 'w') as f:
        f.write('benchmark, type, reach_time, memory_kb, final_states\n')

def write_line(data):
    with open(output_file, 'a') as f:
        f.write('{}, {}, {}, {}, {}\n'.format(*data))

def process_line(data_line):
    duration_ms = float(data_line[45:-3])
    time = duration_ms / 1000.
    return [0, 'RD', time, 0, 0]

def load_file(folder, filename):
    print("processing {}{}".format(folder, filename))
    with open(folder + filename, 'r') as f:
        lines = f.readlines()
        for index, line in enumerate(lines):
            if (line.startswith(look_for)):
                data = process_line(line)
                bench_name = filename[:-4]
                data[0] = bench_name
                write_line(data)
                

def process_data(folder):
    write_header()
    for filename in sorted(os.listdir(folder)):
        load_file(folder, filename)


if __name__ == '__main__':
    process_data('bench_data/its_tools/petrinets_deadlocks/')