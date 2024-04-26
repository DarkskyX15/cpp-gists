<!--
 * @Date: 2024-04-26 17:36:24
 * @Author: DarkskyX15
 * @LastEditTime: 2024-04-26 22:52:15
-->

# 使用C++实现的简单KNN  

包含内容：
- 基于KD树法的KNN对象
- 基于暴力法的KNN对象
- 实现的仅适用于本项目的基础数据集
- 预置的计算曼哈顿距离，欧氏距离的函数
- 预置的默认数据集读取函数对象，支持类csv格式文件的读取
- 实现的默认数据集支持z-score标准化
- 支持将数据集保存为二进制文件
- 支持多线程加速K值的最优选取
- 预置的格式化显示K近邻结果的函数

# 使用例  

`main.cpp`  
```cpp
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
```

# 详细说明  

所有的对象、函数和其他都定义在命名空间`knn`中  
提前说明以下三个模板参数:  
- `__T`：数据集内数据类型
- `__DT`：计算距离时的数据类型
- `__ST`：标签的数据类型

## 数据集相关  

### Record<__T, __ST> (struct)  
代表数据集中的一条数据  
拥有属性：
- `std::vector<__T> vec`
- `__ST state`

### DataSet<__T, __ST> (class)  
所有数据集基类，是抽象类  
子类须实现其所有的方法  

### DefaultDataSet<__T, __ST> (class)  
继承自 `DataSet<__T, __ST>`  
初始化介绍：  
- `DefaultDataSet()` 空初始化
- `DefaultDataSet(long long __dimension)` 指定维度初始化
- `DefaultDataSet(long long __dimension, long long __tot)` 以提供的维度和数据总量初始化  
- `DefaultDataSet(const std::initializer_list<Record<__T, __ST>>& __list)` 以初始化列表初始化


部分方法介绍： 
- `void zScoreNormalization()` 进行z-score法的标准化
- `std::vector<__T> syncNormalization(const std::vector<__T>& __vec)` 将给定的向量与该数据集的标准化同步
- `void clear()` 清空数据集
- `void saveToBin(const char* __target)` 将当前数据集保存为二进制文件
- `void loadFromBin(const char* __source)` 从二进制文件读取数据集  


### ReadLineFunc<__T, __ST> (class)  
数据集的行读取函数对象  
拥有方法：  
- `virtual Record<__T, __ST> operator() (const std::string& __str) const`  
  传入数据集单行的内容，返回一个记录  
  **需要子类实现**  


### DefaultReadLine<__T, __ST> (class)  
继承自`ReadLineFunc<__T, __ST>`，实现了类csv格式文件的读取  
初始化介绍：  
`DefaultReadLine(char __sep, long long __dimension)`  
通过给定的分隔符和数据维数读取，维数不包括标签  

### readDatasetFile (function)  
函数原型：  
- `void readDatasetFile(const char* __filename, DataSet<__T, __ST>& __ds_ref, const ReadLineFunc<__T, __ST>& __line_func_obj)`
- `void readDatasetFile(const char* __filename, DataSet<__T, __ST>& __ds_ref, std::function<Record<__T, __ST>(const std::string&)> __line_func)`

从`__filename`中读取数据集，并保存到`__ds_ref`指定的数据集中，文件的每行通过`__line_func`或`__line_func_obj`进行处理。 

### selectTestGroup (function)  
函数原型：  
`void selectTestGroup(const DataSet<__T, __ST>& __source, DataSet<__T, __ST>& __training_group, DataSet<__T, __ST>& __test_group, long long __test_size)`  

从`__source`指定的数据集中选取`__test_size`条记录分至`__test_group`中作为测试组，其余分至`__training_group`中作为训练组  

## KNN相关  

### BaseKNN<__T, __DT, __ST> (class)  
KNN对象基类  
方法：  
- `std::vector<const Record<__T, __ST>*> getResultContainer()`
  工具函数，配合`auto`使用避免手动指定结果容器的类型  


### Brute<__T, __DT, __ST> (class)  
继承自`BaseKNN<__T, __DT, __ST>`  
基于暴力法的KNN  
有以下默认参数：
- `__T` = `double`
- `__DT` = `__T`
- `__ST` = `int`

构造：  
`Brute(const DataSet<__T, __ST>& __dataset, std::function<__DT(__DT, const std::vector<__T>*)> __weight_func = uniformWeight<__T, __DT>,std::function<__DT(const std::vector<__T>*, const std::vector<__T>*)> __distance_func = euclidean<__T, __DT>)`  

`__dataset`为目标数据集，`__weight_func`为权重函数，默认为与距离一致的权重，`__distance_func`为距离函数，默认为欧氏距离  
可通过传入自定义的权重和距离函数自定义KNN的行为  

方法：  
- `void get(const std::vector<__T>& __vec, int k,std::vector<const Record<__T, __ST>*>& __container)`
  求`__vec`向量的`k`近邻，并将结果传入`__container`  
- `void multiThreadGet(const std::vector<__T>& __vec, int k, int thread_cnt, std::vector<const Record<__T, __ST>*>& __container)`  
  以多线程的方式求`k`近邻，使用线程数为`thread_cnt`，其他参数说明与`get`方法一致  

### KDTree<__T, __DT, __ST> (class)  
继承自`BaseKNN<__T, __DT, __ST>`  
基于KD树加速的KNN，注意该类的`multiThreadGet`方法仅作占位，并不能实现多线程的加速  
其余构造和方法与`Brute<__T, __DT, __ST>`一致  

### testCorrectness (function)  
函数原型：  
`double testCorrectness(BaseKNN<__T, __DT, __ST>& __knn, int __test_k, const DataSet<__T, __ST>& __test_set, int thread_cnt = -1)`  

用`__test_set`测试使用`__test_k`作为参数k时在`__knn`中预测的准确率，并返回。使用`thread_cnt`个线程进行加速  

该函数的多线程实际调用指定`__knn`对象的`multiThreadGet`方法  

### optimizeK (function)  
函数原型：
- `void optimizeK(const __DataSet& data_set, int iteration_cnt, int thread_cnt, std::pair<int, int> __k_range, int __test_size, int __result_size, knn_k_optimization_ret_list& __container)`
- `void optimizeK(const __DataSet& data_set, int iteration_cnt, std::pair<int, int> __k_range, int __test_size, int __result_size, knn_k_optimization_ret_list& __container)`  

针对`data_set`选取预测准确率最优的k，k的备选范围为`__k_range`，测试的次数为`iteration_cnt`，每次测试从`data_set`中选取的作为测试集的数据量为`__test_size`。将最优的`__result_size`个k放入`__container`。若给定了参数`thread_cnt`，则使用多线程加速的重载函数，使用的线程数为`thread_cnt`  

## 其他  

### knn_k_optimization_ret_list (typedef)  
`typedef std::vector<std::pair<int, double>> knn_k_optimization_ret_list`  
最优化k时结果容器的别名  

### euclidean<__T, __DT> (function)  
函数原型：  
`__DT euclidean(const std::vector<__T>* __record, const std::vector<__T>* __sample)`  
预置的欧氏距离函数  

### manhattan<__T, __DT> (function)  
函数原型：  
`__DT manhattan(const std::vector<__T>* __record, const std::vector<__T>* __sample)`  
预置的曼哈顿距离函数  

### uniformWeight<__T, __DT> (function)  
函数原型：  
`__DT uniformWeight(__DT distance, const std::vector<__T>* __record)`  
预置的一致的权重函数  

### Timer (class)  
计时器工具类  

