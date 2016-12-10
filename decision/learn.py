from data_reader import load_data
from sklearn.ensemble import ExtraTreesRegressor
from sklearn.model_selection import train_test_split
from sklearn.externals import joblib

def train_trees(X_train, Y_train):
    print "Training..."
    regressor = ExtraTreesRegressor(\
                    n_estimators=10,\
                    max_features=32,\
                    random_state=0)

    regressor.fit(X_train, Y_train)

    joblib.dump(regressor, "dt.pkl")

    return regressor

def test_trees(regressor, X_test, Y_test):
    print "Predicting..."
    Y_pred = regressor.predict(X_test)

    print Y_test
    print "---------------"
    print Y_pred

if __name__ == "__main__":
    (X, Y) = load_data()

    X_train, X_test, Y_train, Y_test = train_test_split(X, Y, test_size = 0.2, random_state=123)

    print X_train.shape
    print Y_train.shape

    train_len = X_train.shape[0]
    test_len  = X_test.shape[0]

    print str(train_len) + " data used to train"
    print str(test_len) + " data used to test"

    regressor = train_trees(X, Y)
    #test_trees(regressor, X, Y)