import os
import numpy as np

matrix_folders = {  ('beem','vn-bdd') : 'models/beem/matrices/bdds/vanilla/', 
                    ('beem','sl-bdd') : 'models/beem/matrices/bdds/sloan/',
                    ('ptri','vn-bdd') : 'models/petrinets/matrices/bdds/vanilla/',
                    ('ptri','sl-bdd') : 'models/petrinets/matrices/bdds/sloan/',
                    ('prom','vn-bdd') : 'models/promela/matrices/bdds/vanilla/',
                    ('prom','sl-bdd') : 'models/promela/matrices/bdds/sloan/',
                    ('beem','vn-ldd') : 'models/beem/matrices/ldds/vanilla/', 
                    ('beem','sl-ldd') : 'models/beem/matrices/ldds/sloan/',
                    ('ptri','vn-ldd') : 'models/petrinets/matrices/ldds/vanilla/',
                    ('ptri','sl-ldd') : 'models/petrinets/matrices/ldds/sloan/',
                    ('prom','vn-ldd') : 'models/promela/matrices/ldds/vanilla/',
                    ('prom','sl-ldd') : 'models/promela/matrices/ldds/sloan/',}

verbose = True


def info(str, end='\n'):
    if (verbose):
        print(str, end=end)


def load_matrix(filepath):
    info("reading {}".format(filepath))
    file = open(filepath, 'r')
    matrix = []
    max_length = 0
    for line in file:
        row = []
        for char in line:
            if (char == '+' or char == 'r' or char == 'w'):
                row.append(1)
            elif (char == '-'):
                row.append(0)
        matrix.append(row)
        max_length = max(max_length, len(row))
    # for LDDs this can produce ragged lists, so extend with 0's
    for i, row in enumerate(matrix):
        row.extend([0] * (max_length - len(row)) )
    return np.array(matrix)


def analyze_matrices():   
    densities = []
    for key in matrix_folders.keys():
        folder = matrix_folders[key]
        for filename in os.listdir(folder):
            m = load_matrix(folder + filename)
            density = np.sum(m) / m.size
            densities.append((density, folder + filename))
    print(*densities, sep='\n')
    densities = sorted(densities, key=lambda tup: tup[0])
    print(*densities, sep='\n')


if __name__ == '__main__':
    analyze_matrices()
