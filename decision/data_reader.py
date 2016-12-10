import numpy
import struct
import random


def load_data(X_path = "X.bin", Y_path = "Y.bin"):
    random.seed(233345)

    f = open(X_path, 'rb')
    x_list = []
    while True:
        flag = False
        list = []
        for i in range(36):
            x_bytes = f.read(4)
            if not x_bytes:
                flag = True;
                break;
            x = struct.unpack('i', x_bytes)[0];
            list.append(x)

        if flag:
            break;

        # valid data
        x_list.append(numpy.array(list))
    f.close()

    f = open(Y_path, 'rb')
    y_list = []
    while True:
        flag = False
        list = []
        for i in range(5 * 41):
            y_bytes = f.read(4)
            if not y_bytes:
                flag = True;
                break;
            y = struct.unpack('f', y_bytes)[0];
            list.append(y)

        if flag:
            break;

        # valid data
        y_list.append(numpy.array(list))
    f.close()

    X = numpy.array(x_list)
    Y = numpy.array(y_list)

    return (X, Y)


if __name__ == "__main__":
    (X, Y) = load_data()
    print X
    print Y
