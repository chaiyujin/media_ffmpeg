from data_reader import load_data
from sklearn.ensemble import ExtraTreesRegressor
from sklearn.externals import joblib

if __name__ == "__main__":
    (X, Y) = load_data("x_input", "y_input")

    regressor = joblib.load("dt.pkl")

    y_pred = regressor.predict(X)
    rows = y_pred.shape[0]
    cols = y_pred.shape[1]
    f = open("output.txt", "w")
    f.write(str(rows) + " " + str(cols) + "\n")
    for i in range(rows):
        for j in range(cols):
            f.write(str(y_pred[i][j]) + " ")
        f.write("\n")
    f.close()