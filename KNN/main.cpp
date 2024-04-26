/*
 * @Date: 2024-03-28 20:53:03
 * @Author: DarkskyX15
 * @LastEditTime: 2024-04-26 17:24:04
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
    readDatasetFile(".\\iris\\iris.data", data_set, 
                    DefaultReadLine<DataType, LabelType>(',', dimension));
    data_set.zScoreNormalization();
    data_set.saveToBin(".\\data.bin");
    // data_set.loadFromBin(".\\data.bin");
    tm.start();
    optimizeK<DefaultDataSet<DataType, LabelType>, 
            KDTree<DataType, DistanceType, LabelType>>
            (data_set, 20, 20, {1, 50}, 20, 5, bkq);
    tm.end("Optimization");

    KDTree<DataType, DistanceType, LabelType> kd_tree(data_set);
    auto vec = data_set.syncNormalization({5.1, 3.5, 1.8, 0.2});
    auto result = kd_tree.getResultContainer();

    kd_tree.get(vec, bkq.at(0).first, result);
    collectResult(result);
    return 0;
}