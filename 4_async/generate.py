import random, math, sys


def generate(file_out, span):
    arr = []
    for _ in range(span):
        i = int(random.random() * 1000)
        arr.append(str(i))
        arr.append(str(-i))

    with open(file_out, 'w') as file:
        file.write('\n'.join(arr))


if __name__ == '__main__':
    generate(sys.argv[1], 500)
