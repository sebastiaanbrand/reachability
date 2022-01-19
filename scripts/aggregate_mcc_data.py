import os

look_for1 = "Actual read output values :"
look_for2 = "##teamcity[testFinished name='all' duration='"

output_file = 'bench_data/its_tools/its_tools_petrinets_deadlocks.csv'

def write_header():
    with open(output_file, 'w') as f:
        f.write('benchmark, type, reach_time, memory_kb, deadlocks\n')

def write_line(data):
    with open(output_file, 'a') as f:
        f.write('{}, {}, {}, {}, {}\n'.format(*data))

def process_line1(data_line):
    dl_string = data_line[21:-1]
    print("[{}]".format(dl_string))
    if (dl_string == 'TRUE'):
        print("it's true")
        return 1
    elif (dl_string == 'FALSE'):
        print("it's false")
        return 0
    else:
        return -1


def process_line2(data_line):
    duration_ms = float(data_line[45:-3])
    time = duration_ms / 1000.
    return time


def load_file(folder, filename):
    print("processing {}{}".format(folder, filename))
    with open(folder + filename, 'r') as f:
        lines = f.readlines()
        data = [filename[:-4], 'RD', 0, 0, -1]
        for index, line in enumerate(lines):
            if (line.startswith(look_for1)):
                data[4] = process_line1(lines[index+1])
            if (line.startswith(look_for2)):
                data[2] = process_line2(line)
        write_line(data)
                

def process_data(folder):
    write_header()
    for filename in sorted(os.listdir(folder)):
        load_file(folder, filename)


if __name__ == '__main__':
    process_data('bench_data/its_tools/petrinets_deadlocks/')