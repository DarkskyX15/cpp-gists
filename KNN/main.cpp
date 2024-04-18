/*
 * @Date: 2024-03-28 20:53:03
 * @Author: DarkskyX15
 * @LastEditTime: 2024-04-18 14:22:53
 */

#include "KNN.hpp"

using namespace knn;

int main() {
    typedef std::string LabelType;
    typedef double DataType;
    typedef double DistanceType;
    const long long dimension = 4;
    Timer tm;

    knn_k_optimization_ret_list bkq;
    DefaultDataSet<DataType, LabelType> data_set(dimension);
    tm.start();
    readDatasetFile(".\\iris\\iris.data", data_set, 
                    DefaultReadLine<DataType, LabelType>(',', dimension));
    data_set.zScoreNormalization();
    tm.start();
    optimizeK<DefaultDataSet<DataType, LabelType>, 
            KDTree<DistanceType, DataType, LabelType>>
            (data_set, 20, 20, {1, 50}, 20, 5, bkq);
    tm.end("Optimization");

    std::vector<const Record<DataType, LabelType>*> result;
    KDTree<DistanceType, DataType, LabelType> knn(data_set);
    auto vec = data_set.syncNormalization({5.1, 3.5, 1.8, 0.2});

    knn.get(vec, bkq.at(0).first, result);
    collectResult(result);
    return 0;
}