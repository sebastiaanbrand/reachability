# This script combines the wall times for encoding the hmorph LDDs (mostly
# negligible) and the wall time for running REACH on them.
import os
import sys

look_for1 = "pnml-encode: Result symbolic LTS written to"
look_for2 = "real\t"

look_for_ddmc1 = "Writing stats to"
look_for_ddmc2 = "real\t"

encode_folder = 'models/petrinets/static_ldds/sloan/logs/'
outputfile = ''

encode_time = {}
ddmc_time = {}
both_time = {}

def write_header():
    with open(output_file, 'w') as f:
        f.write('benchmark, encode_time, ddmc_time\n')

def write_to_csv():
    write_header()
    with open(output_file, 'a') as f:
        for key, value in both_time.items():
            f.write('{}, {}, {}\n'.format(key, value[0], value[1]))

def parse_time(line):
    time_str = line.split()[1]
    time_str = time_str.split('m')
    time_min = float(time_str[0])
    time_sec = float(time_str[1][:-1].replace(',','.'))
    return time_min * 60 + time_sec

def parse_encode_file(folder, filename):
    print("processing {}".format(folder + filename))
    with open(folder + filename, 'r') as f:
        lines = f.readlines()
        wrote_dd = False
        for index, line in enumerate(lines):
            if (line.startswith(look_for1)):
                wrote_dd = True
            if (line.startswith(look_for2)):
                time = parse_time(line)
        if (not wrote_dd):
            time = 'DNF'
        encode_time[filename[:-4]] = time

def parse_ddmc_file(folder, filename):
    print("processing {}".format(folder + filename))
    with open(folder + filename, 'r') as f:
        lines = f.readlines()
        finished = False
        for index, line in enumerate(lines):
            if (look_for_ddmc1 in line):
                finished = True
            if (line.startswith(look_for_ddmc2)):
                time = parse_time(line)
        if (not finished):
            time = 'DNF'
        ddmc_time[filename[:-15]] = time

def process_encode_data(folder):
    for filename in sorted(os.listdir(folder)):
        if (filename.endswith('.log')):
            parse_encode_file(folder, filename)

def process_ddmc_data(folder):
    for filename in sorted(os.listdir(folder)):
        if (filename.endswith('.log')):
            parse_ddmc_file(folder, filename)

def merge_dicts():
    for d in (encode_time, ddmc_time):
        for key in d.keys():
            both_time[key] = ['-', '-']
    for key, value in encode_time.items():
        both_time[key][0] = value
    for key, value in ddmc_time.items():
        both_time[key][1] = value


def parse_args():

    data_folder = None
    if (len(sys.argv) <= 1):
        print("argument 1 (path to data folder) missing")
        exit()
    data_folder = sys.argv[1]
    return data_folder


if __name__ == '__main__':

    data_folder = parse_args()
    output_file = data_folder + 'pnml_encode_time_ldd_hmorph.csv'

    process_encode_data(encode_folder)
    process_ddmc_data(data_folder + 'logs/')
    merge_dicts()
    write_to_csv()
