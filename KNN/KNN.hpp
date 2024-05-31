/*
 * @Date: 2024-03-28 18:19:40
 * @Author: DarkskyX15
 * @LastEditTime: 2024-05-31 12:43:24
 */
#ifndef DS_KNN_HPP
#define DS_KNN_HPP

#if __cplusplus >= 200704L

#include <type_traits>
#include <future>
#include <thread>
#include <chrono>
#include <random>
#include <string>
#include <vector>
#include <queue>
#include <math.h>
#include <iostream>
#include <fstream>
#include <functional>
#include <utility>
#include <initializer_list>
#include <algorithm>
#include <charconv>
#include <unordered_map>
#include <set>
#include <iomanip>
#include <sstream>

/// @brief 
namespace knn {

    typedef std::vector<std::pair<int, double>> knn_ranged_k_ret_list;

    /// @brief 简单的用于计时的工具类
    class Timer { 
        public:
        /// @brief 获取开始时刻
        inline void start() {
            t_start = std::chrono::steady_clock::now();
        }
        /// @brief 获取结束时刻并输出，单位us
        inline void end(const char* __action) {
            t_end = std::chrono::steady_clock::now();
            interval = std::chrono::duration_cast<std::chrono::microseconds>(t_end - t_start);
            std::cout << __action << " took " << Timer::interval.count() << "us." << '\n';
        }
        private:
        std::chrono::time_point<std::chrono::steady_clock> t_start, t_end;
        std::chrono::microseconds interval;
    };

    /// @brief 一条数据记录
    /// @tparam __T 向量的数据类型
    /// @tparam __ST 类别的数据类型
    template<class __T, class __ST>
    struct Record {
        std::vector<__T> vec;
        __ST state;
    };

    /// @brief KDTree的节点
    /// @tparam __T 向量的数据类型
    /// @tparam __ST 类别的数据类型
    template<class __T, class __ST>
    struct KDNode {
        const Record<__T, __ST>* rec_ptr;
        KDNode<__T, __ST> *left_ptr, *right_ptr, *father;
    };

    /// @brief 数据集基类
    /// @tparam __T 向量中的数据类型
    /// @tparam __ST 数据分类的数据类型
    template<class __T, class __ST>
    class DataSet {
        public:
        /// @brief 返回编号对应的数据记录的指针
        /// @param __index 编号
        /// @return 指向对应数据的指针
        virtual Record<__T, __ST>* getRef(long long __index) = 0;
        virtual const Record<__T, __ST>* getRef(long long __index) const = 0;
        /// @brief 返回数据维度
        /// @return 表维度的`long long`
        virtual long long getDimension() const = 0;
        /// @brief 返回数据集大小
        /// @return 表大小的`long long`
        virtual long long dataSize() const = 0;
        virtual inline void appendRecord(const std::vector<__T>& __vec, const __ST __state) = 0;
        virtual inline void appendRecord(const Record<__T, __ST>& __record) = 0;
        virtual void clear() = 0;
        virtual std::vector<__T> syncNormalization(const std::vector<__T>& __vec) const = 0;
    };

    /// @brief 实现的基本数据集
    /// @tparam __T 向量中的数据类型
    /// @tparam __ST 标签的数据类型
    template<class __T, class __ST>
    class DefaultDataSet : public DataSet<__T, __ST> {
        public:
        DefaultDataSet() = default;
        /// @brief 以给定维度初始化
        /// @param __dimension 维度
        DefaultDataSet(long long __dimension) : data() {
            dimension = __dimension;
            tot_samples = 0;
        }
        /// @brief 以提供的维度和总量初始化
        /// @param __dimension 维度
        /// @param __tot 总数据数
        DefaultDataSet(long long __dimension, long long __tot) : data() {
            dimension = __dimension;
            tot_samples = 0;
            data.reserve(__tot);
        }
        /// @brief 列表初始化
        /// @param __list 初始化列表
        DefaultDataSet(const std::initializer_list<Record<__T, __ST>>& __list) : data() {
            dimension = __list.begin()->vec.size();
            tot_samples = 0;
            for (auto it = __list.begin(); it != __list.end(); ++it, ++tot_samples) {
                data.push_back(*it);
            }
        }
        DefaultDataSet(const DefaultDataSet& __dataset) = default;
        
        /// @brief 添加数据记录
        /// @param __record 记录
        inline void appendRecord(const std::vector<__T>& __vec, const __ST __state) override {
            tot_samples += 1;
            data.push_back(Record<__T, __ST>{__vec, __state});
        }
        inline void appendRecord(const Record<__T, __ST>& __record) override {
            tot_samples += 1;
            data.push_back(__record);
        }

        void saveToBin(const char* __target) const {
            std::ofstream file_out(__target, std::ios::out | std::ios::binary);
            // Header
            binaryWrite(tot_samples, file_out);
            binaryWrite(dimension, file_out);
            if constexpr (std::is_integral_v<__ST> || std::is_floating_point_v<__ST>) {
                binaryWrite(false, file_out);
                for (auto& rec : data) {
                    for (auto& feature : rec.vec) {
                        binaryWrite(feature, file_out);
                    }
                    binaryWrite(rec.state, file_out);
                }
            } else {
                binaryWrite(true, file_out);
                std::unordered_map<std::string, int> tags;
                int tag_id = 0;
                for (auto& element : data) {
                    auto it = tags.find(element.state);
                    if (it == tags.end()) {
                        tags.insert(std::make_pair(element.state, tag_id));
                        tag_id += 1;
                    }
                }
                binaryWrite((int)tags.size(), file_out);
                for (auto& tag : tags) {
                    binaryWrite((int)tag.first.size(), file_out);
                    file_out.write(tag.first.c_str(), tag.first.size());
                    binaryWrite(tag.second, file_out);
                }
                for (auto& rec : data) {
                    for (auto& feature : rec.vec) {
                        binaryWrite(feature, file_out);
                    }
                    binaryWrite(tags[rec.state], file_out);
                }
            }
            if (normalized) {
                binaryWrite(true, file_out);
                for (auto ele_u : __u) binaryWrite(ele_u, file_out);
                for (auto ele_a : __a) binaryWrite(ele_a, file_out);
            } else {
                binaryWrite(false, file_out);
            }
            file_out.close();
        }

        void loadFromBin(const char* __source) {
            std::ifstream fin(__source, std::ios::in | std::ios::binary);
            // Header
            binaryRead(tot_samples, fin);
            binaryRead(dimension, fin);
            bool state_type;
            binaryRead(state_type, fin);
            data.reserve(tot_samples);
            std::vector<__T> features;
            if constexpr (std::is_integral_v<__ST> || std::is_floating_point_v<__ST>) {
                for (int i = 0; i < tot_samples; ++i) {
                    features.clear();
                    features.reserve(dimension);
                    for (int j = 0; j < dimension; ++j) {
                        __T x;
                        binaryRead(x, fin);
                        features.push_back(x);
                    }
                    __ST s;
                    binaryRead(s, fin);
                    data.push_back({features, s});
                }
            } else {
                std::unordered_map<int, std::string> tags;
                int tag_size;
                binaryRead(tag_size, fin);
                for (int i = 0; i < tag_size; ++i) {
                    int s_size;
                    binaryRead(s_size, fin);
                    char* temp = new char[s_size + 1];
                    fin.read(temp, s_size);
                    temp[s_size] = '\000';
                    std::string x(temp);
                    binaryRead(s_size, fin);
                    tags.insert(std::make_pair(s_size, x));
                    delete[] temp;
                }
                for (int i = 0; i < tot_samples; ++i) {
                    features.clear();
                    features.reserve(dimension);
                    for (int j = 0; j < dimension; ++j) {
                        __T x;
                        binaryRead(x, fin);
                        features.push_back(x);
                    }
                    int s;
                    binaryRead(s, fin);
                    data.push_back({features, tags[s]});
                }
            }
            binaryRead(normalized, fin);
            if (normalized) {
                __u.reserve(dimension);
                __a.reserve(dimension);
                __T temp_data;
                for (int i = 0; i < dimension; ++i) {
                    binaryRead(temp_data, fin);
                    __u.push_back(temp_data);
                }
                for (int i = 0; i < dimension; ++i) {
                    binaryRead(temp_data, fin);
                    __a.push_back(temp_data);
                }
            }
        }

        /// @brief z-score法标准化
        void zScoreNormalization() {
            normalized = true;
            __u.reserve(dimension);
            __a.reserve(dimension);
            for (int dim = 0; dim < dimension; ++dim) {
                double __mean = 0.0, __std_o = 0.0;
                for (int i = 0; i < tot_samples; ++i) {
                    __mean += data[i].vec[dim];
                }
                __mean /= static_cast<double>(tot_samples);
                __u.push_back(static_cast<__T>(__mean));
                for (int i = 0; i < tot_samples; ++i) {
                    __std_o += (data[i].vec[dim] - __mean) * (data[i].vec[dim] - __mean);
                }
                __std_o /= static_cast<double>(tot_samples);
                __std_o = std::sqrt(__std_o);
                __a.push_back(static_cast<__T>(__std_o));

                for (int i = 0; i < tot_samples; ++i) {
                    data[i].vec[dim] = (data[i].vec[dim] - __mean) / __std_o;
                }
            }
        }

        /// @brief 将给定的向量与该数据集的标准化同步
        /// @param __vec 给定向量
        /// @return 标准化后的向量
        std::vector<__T> syncNormalization(const std::vector<__T>& __vec) const override {
            if (!normalized) return __vec;
            std::vector<__T> temp;
            temp.reserve(__vec.size());
            for (int i = 0; i < __vec.size(); ++i) {
                temp.push_back((__vec[i] - __u[i]) / __a[i]);
            }
            return temp;
        }

        void clear() override {
            data.clear();
            tot_samples = 0;
        }

        Record<__T, __ST>* getRef(long long __index) override {
            return &data.at(__index);
        }
        const Record<__T, __ST>* getRef(long long __index) const override {
            return &data.at(__index);
        }

        long long getDimension() const override {
            return this->dimension;
        }
        long long dataSize() const override {
            return this->tot_samples;
        }

        private:
        template<class __WT>
        void binaryWrite(const __WT& __data, std::ofstream& __ofs) const {
            const char* x = reinterpret_cast<const char*>(&__data);
            __ofs.write(x, sizeof(__WT));
        }
        template<class __RT>
        void binaryRead(__RT& __val, std::ifstream& __ifs) const {
            char* x = new char[sizeof(__RT)];
            __ifs.read(x, sizeof(__RT));
            __val = *reinterpret_cast<__RT*>(x);
            delete[] x;
        }

        bool normalized = false;
        std::vector<__T> __u, __a;
        std::vector< Record<__T, __ST> > data;
        long long dimension, tot_samples;
    };

    /// @brief 一致的权重
    template<class __T, class __DT>
    __DT uniformWeight(__DT distance, const std::vector<__T>* __record) {
        return distance;
    }

    /// @brief 欧氏距离
    template<class __T, class __DT>
    __DT euclidean(const std::vector<__T>* __record, const std::vector<__T>* __sample) {
        __DT dis{0}, x, z;
        for (auto it = __record->begin(), sit = __sample->begin(); 
            it != __record->end() && sit != __sample->end(); ++it, ++sit) {
            x = static_cast<__DT>(*it); z = static_cast<__DT>(*sit);
            z -= x; z *= z; dis += z;
        }
        return std::sqrt(dis);
    }
    /// @brief 曼哈顿距离
    template<class __T, class __DT>
    __DT manhattan(const std::vector<__T>* __record, const std::vector<__T>* __sample) {
        __DT dis{0};
        for (auto it = __record->begin(), sit = __sample->begin();
            it != __record->end() && sit != __record->end(); ++it, ++sit) {
                dis += std::abs(static_cast<__DT>(*it - *sit));
        }
        return dis;
    }

    template<class __T, class __DT, class __ST>
    class BaseKNN {
        public:
        virtual void get(const std::vector<__T>& __vec, int k,
                        std::vector<const Record<__T, __ST>*>& __container) = 0;
        virtual void multiThreadGet(const std::vector<__T>& __vec, int k, int thread_cnt,
                                    std::vector<const Record<__T, __ST>*>& __container) = 0;
        std::vector<const Record<__T, __ST>*> getResultContainer() {
            return std::vector<const Record<__T, __ST>*>();
        }
        virtual const DataSet<__T, __ST>* getDatasetRef() const = 0;
    };

    /// @brief 暴力法KNN
    /// @tparam __T 数据集中的数据类型 `Type`
    /// @tparam __DT 距离计算过程中的数据类型 `Distance Type`
    /// @tparam __ST 数据分类的数据类型 `State Type`
    template<class __T = double, class __DT = __T, class __ST = int>
    class Brute : public BaseKNN<__T, __DT, __ST> {
        public:
        /// @brief 以指定数据集，权重函数和距离函数初始化
        /// @param __dataset 数据集
        /// @param __weight_func 权重函数，类型为`__DT(__DT, const std::vector<__T>*)`
        /// @param __distance_func 距离函数，类型为`__DT(const std::vector<__T>*, const std::vector<__T>*)`
        Brute(const DataSet<__T, __ST>& __dataset, 
            std::function<__DT(__DT, const std::vector<__T>*)> __weight_func = uniformWeight<__T, __DT>,
            std::function<__DT(const std::vector<__T>*, const std::vector<__T>*)> __distance_func = euclidean<__T, __DT>) {
            data_ptr = &__dataset;
            weight_func = __weight_func;
            distance_func = __distance_func;
        }
        /// @brief 获取结果
        /// @param __vec 预测的向量
        /// @param k 参数k
        /// @param __container 储存结果的容器，类型为`std::vector<const Record<__T, __ST>*>`
        void get(const std::vector<__T>& __vec, int k,
                std::vector<const Record<__T, __ST>*>& __container) override {
            typedef std::pair<const Record<__T, __ST>*, __DT> data_pair;
            auto cmp = [](const data_pair& left, const data_pair& right){
                if (left.second < right.second) return true;
                return false;
            };
            std::priority_queue<data_pair, std::vector<data_pair>, decltype(cmp)> kq(cmp);
            
            const Record<__T, __ST>* __rec_ptr;
            for (long long i = 0; i < data_ptr->dataSize(); ++i) {
                __rec_ptr = data_ptr->getRef(i);
                __DT val = weight_func(distance_func(&(__rec_ptr->vec), &__vec),
                                    &(__rec_ptr->vec));
                if (kq.size() < k) {
                    kq.push(std::make_pair(__rec_ptr, val));
                } else {
                    if (val < kq.top().second) {
                        kq.pop();
                        kq.push(std::make_pair(__rec_ptr, val));
                    }
                }
            }
            int size = kq.size();
            __container.resize(size);
            while (size > 0) {
                __container[size - 1] = kq.top().first;
                kq.pop(); --size;
            }
        }

        /// @brief 多线程查询
        /// @param __vec 要查询的向量
        /// @param k 参数k
        /// @param thread_cnt 使用的线程数
        /// @param __container 保存结果的容器，类型为`std::vector<const Record<__T, __ST>*>`
        void multiThreadGet(const std::vector<__T>& __vec, int k, int thread_cnt,
                            std::vector<const Record<__T, __ST>*>& __container) override {
            std::vector<std::thread*> thread_pool;
            std::vector<std::future<sub_ret*>> sub_task_rets;
            thread_pool.reserve(thread_cnt);
            sub_task_rets.reserve(thread_cnt);
            long long unit_len = data_ptr->dataSize() / thread_cnt;
            long long left = 0, right = 0;

            for (int i = 0; i < thread_cnt; ++i) {
                if (i == thread_cnt - 1) {
                    right = data_ptr->dataSize() - 1;
                } else {
                    right = left + unit_len - 1;
                }
                std::promise<sub_ret*> __promise;
                sub_task_rets.push_back(__promise.get_future());
                auto ptr = new std::thread(&subTask, this, left, right, 
                                           __vec, std::move(__promise), k);
                thread_pool.push_back(ptr);
                left = right + 1;
            }

            sub_ret results;
            for (int i = 0; i < thread_cnt; ++i) {
                thread_pool.at(i)->join();
                sub_ret* ret_ptr = sub_task_rets[i].get();
                sub_pair __sub_pair;
                while (ret_ptr->size()) {
                    __sub_pair = ret_ptr->top();
                    if (results.size() < k) {
                        results.push(__sub_pair);
                    } else if (results.top().second > __sub_pair.second) {
                        results.pop();
                        results.push(__sub_pair);
                    }
                    ret_ptr->pop();
                }

                delete thread_pool.at(i);
                delete ret_ptr;
                thread_pool.at(i) = nullptr;
            }

            int size = results.size();
            __container.resize(size);
            while (size > 0) {
                __container[size - 1] = results.top().first;
                results.pop(); --size;
            }
        }

        const DataSet<__T, __ST>* getDatasetRef() const override {
            return data_ptr;
        }

        private:

        typedef std::pair<const Record<__T, __ST>*, __DT> sub_pair;
        struct __Compare {
            bool operator()(const sub_pair& left, const sub_pair& right) {
                if (left.second < right.second) return true;
                return false;
            }
        };
        typedef std::priority_queue<sub_pair, std::vector<sub_pair>, __Compare> sub_ret;

        void subTask(long long left, long long right, std::vector<__T> __vec,
                     std::promise<sub_ret*> __promise, int k) {
            sub_ret* ptr = new sub_ret();
            const Record<__T, __ST>* __rec_ptr;
            for (long long index = left; index <= right; ++index) {
                __rec_ptr = data_ptr->getRef(index);
                __DT distance = weight_func(distance_func(&(__rec_ptr->vec), &__vec),
                                            &(__rec_ptr->vec));
                if (ptr->size() < k) {
                    ptr->push(std::make_pair(__rec_ptr, distance));
                } else if (distance < ptr->top().second) {
                    ptr->pop();
                    ptr->push(std::make_pair(__rec_ptr, distance));
                }
            }
            __promise.set_value(ptr);
        }
        
        const DataSet<__T, __ST>* data_ptr;
        std::function<__DT(__DT, const std::vector<__T>*)> weight_func;
        std::function<__DT(const std::vector<__T>*, const std::vector<__T>*)> distance_func;
    };

    /// @brief 向量排序使用的比较
    template<class __T, class __ST>
    class KDSort {
        public:
        KDSort(long long __dimension) {
            dim = __dimension;
        }
        bool operator() (const Record<__T, __ST>* left, const Record<__T, __ST>* right) {
            if (left->vec.at(dim) < right->vec.at(dim)) return true;
            return false;
        }
        private:
        long long dim;
    };

    /// @brief K-Dimension Tree法KNN
    /// @tparam __T 数据集中的数据类型 `Type`
    /// @tparam __DT 距离计算过程中的数据类型 `Distance Type`
    /// @tparam __ST 数据分类的数据类型 `State Type`
    template<class __T = double, class __DT = __T, class __ST = int>
    class KDTree : public BaseKNN<__T, __DT, __ST>{
        public:
        /// @brief 以指定数据集，权重函数和距离函数初始化
        /// @param __dataset 数据集
        /// @param __weight_func 权重函数，类型为`__DT(__DT, const std::vector<__T>*)`
        /// @param __distance_func 距离函数，类型为`__DT(const std::vector<__T>*, const std::vector<__T>*)`
        KDTree(const DataSet<__T, __ST>& __dataset, 
            std::function<__DT(__DT, const std::vector<__T>*)> __weight_func = uniformWeight<__T, __DT>,
            std::function<__DT(const std::vector<__T>*, const std::vector<__T>*)> __distance_func = euclidean<__T, __DT>) {
            data_ptr = &__dataset;
            dimension = data_ptr->getDimension();
            std::vector<const Record<__T, __ST>*> __vec;
            root = new KDNode<__T, __ST>();

            weight_func = __weight_func;
            distance_func = __distance_func;
            __vec.reserve(data_ptr->dataSize());
            root->father = root;
            for (long long i = 0; i < data_ptr->dataSize(); ++i) {
                __vec.push_back(data_ptr->getRef(i));
            }

            sort(__vec.begin(), __vec.end(), KDSort<__T, __ST>(0));
            long long mid = (__vec.size() >> 1);
            root->rec_ptr = __vec[mid];
            root->left_ptr = construct(__vec, 1, __vec.begin(), __vec.begin() + mid, root);
            root->right_ptr = construct(__vec, 1, __vec.begin() + mid + 1, __vec.end(), root);
        }
        ~KDTree(){
            if (root != nullptr) {
                deconstruct(root);
                root = nullptr;
            }
        }

        const DataSet<__T, __ST>* getDatasetRef() const override {
            return data_ptr;
        }
        
        /// @brief 获取结果，不保证返回数量为k
        /// @param __vec 预测的向量
        /// @param k 参数k
        /// @param __container 储存结果的容器，类型为`std::vector<const Record<__T, __ST>*>`
        void get(const std::vector<__T>& __vec, int k, 
                std::vector<const Record<__T, __ST>*>& __container) override {
            __container.clear();

            tpk_type tpk;
            searchTree(root, __vec, 0, tpk, k);
            __container.resize(tpk.size());
            while (tpk.size()) {
                __container[tpk.size() - 1] = tpk.top().first;
                tpk.pop();
            }
        }
        
        /// @brief 仅作为方法占位，KDTree不提供多线程查询
        void multiThreadGet(const std::vector<__T>& __vec, int k, int thread_cnt,
                            std::vector<const Record<__T, __ST>*>& __container) override {
            this->get(__vec, k, __container);
        }

        private:
        typedef typename std::vector<const Record<__T, __ST>*>::iterator vec_it;
        typedef std::pair<const Record<__T, __ST>*, __DT> d_pair;
        struct KDHeap {
            bool operator()(const d_pair& left, const d_pair& right) {
                if (left.second < right.second) return true;
                return false;
            }
        };
        typedef std::priority_queue<d_pair, std::vector<d_pair>, KDHeap> tpk_type;

        KDNode<__T, __ST>* construct(std::vector<const Record<__T, __ST>*>& __vec,
                                    int depth, vec_it left, vec_it right, KDNode<__T, __ST>* fa) {
            if (left == right) return nullptr;
            KDNode<__T, __ST>* ptr = new KDNode<__T, __ST>();
            ptr->father = fa;
            if (right - left == 1) {
                ptr->rec_ptr = *left;
            } else {
                std::sort(left, right, KDSort<__T, __ST>(depth % dimension));
                long long mid = ((right - left) >> 1);
                ptr->rec_ptr = *(left + mid);
                ptr->left_ptr = construct(__vec, depth + 1, left, left + mid, ptr);
                ptr->right_ptr = construct(__vec, depth + 1, left + mid + 1, right, ptr);
            }
            return ptr;
        }
        void deconstruct(KDNode<__T, __ST>* ptr) {
            if (ptr->left_ptr != nullptr) deconstruct(ptr->left_ptr);
            if (ptr->right_ptr != nullptr) deconstruct(ptr->right_ptr);
            delete ptr;
        }
        void searchTree(const KDNode<__T, __ST>* __present, const std::vector<__T>& __vec,
                    int depth, tpk_type& __tpk, const int k) {

            long long index = depth % dimension;
            __DT distance = weight_func(distance_func(&(__present->rec_ptr->vec), &__vec), &__vec);
            bool left_flg = (__vec[index] < __present->rec_ptr->vec[index]) ? true : false;

            if (__present->left_ptr == nullptr && __present->right_ptr == nullptr) {
                if (__tpk.size() < k) __tpk.push(std::make_pair(__present->rec_ptr, distance));
                else if (distance < __tpk.top().second) {
                    __tpk.pop();
                    __tpk.push(std::make_pair(__present->rec_ptr, distance));
                }
                return ;
            }
            
            if (left_flg) {
                if (__present->left_ptr != nullptr) {
                    searchTree(__present->left_ptr, __vec, depth + 1, __tpk, k);
                }
            } else {
                if (__present->right_ptr != nullptr) {
                    searchTree(__present->right_ptr, __vec, depth + 1, __tpk, k);
                }
            }
            
            if (__tpk.size() < k) {
                __tpk.push(std::make_pair(__present->rec_ptr, distance));
            } else if (distance < __tpk.top().second) {
                __tpk.pop();
                __tpk.push(std::make_pair(__present->rec_ptr, distance));
            }

            bool next_flg = false;
            if (std::abs(__vec[index] - __present->rec_ptr->vec[index]) < __tpk.top().second) {
                next_flg = true;
            }

            if (next_flg) {
                if (left_flg) {
                    if (__present->right_ptr != nullptr) {
                        searchTree(__present->right_ptr, __vec, depth + 1, __tpk, k);
                    }
                } else {
                    if (__present->left_ptr != nullptr) {
                        searchTree(__present->left_ptr, __vec, depth + 1, __tpk, k);
                    }
                }
            } else return;
        }

        long long dimension;
        KDNode<__T, __ST>* root;
        const DataSet<__T, __ST>* data_ptr;
        std::function<__DT(__DT, const std::vector<__T>*)> weight_func;
        std::function<__DT(const std::vector<__T>*, const std::vector<__T>*)> distance_func;
    };

    /// @brief 分割字符串
    /// @param __str 原字符串
    /// @param __container 存储结果的容器
    /// @param __sep 分割字符
    void splitString(const std::string& __str, 
                    std::vector<std::string>& __container,
                    const char __sep = -1) {
        std::string temp;
        __container.clear();
        if (__sep < 0) {
            std::stringstream s_sep(__str, std::ios::in);
            while(s_sep >> temp) {
                __container.push_back(temp);
            }
        } else {
            long long left = 0, right = 0;
            long long send = __str.size();
            while (left < send && right < send) {
                if (__str.at(right) == __sep) {
                    temp = __str.substr(left, right - left);
                    if (temp.size()) __container.push_back(temp);
                    left = right + 1;
                }
                right += 1;
            }
            temp = __str.substr(left, right - left);
            if (temp.size()) __container.push_back(temp);
        }
    }

    template <typename __T>
    void fromStr(const std::string& __str, __T& value) {
        if constexpr (std::is_integral_v<__T>) {
            value = 0;
            int is_minus = 0;
            if (__str[0] == '-') is_minus = 1;
            for (auto it = __str.begin() + is_minus; it != __str.end(); ++it) {
                value = (value << 1) + (value << 3);
                value += *it - '0';
            }
            if (is_minus) value *= -1;
        } else if constexpr (std::is_floating_point_v<__T>) {
            value = 0.0;
            int is_minus = 0;
            if (__str[0] == '-') is_minus = 1;
            __T base = 1.0;
            bool floating = false;
            for (auto it = __str.begin() + is_minus; it != __str.end(); ++it) {
                if (!floating) {
                    if (*it == '.') {
                        floating = true;
                        continue;
                    }
                    else {
                        value *= 10.0;
                        value += static_cast<__T>(*it - '0');
                    }
                } else {
                    base /= 10.0;
                    value += base * static_cast<__T>(*it - '0');
                }
            }
            if (is_minus) value *= -1.0;
        } else {
            value = __str;
        }
    }

    template<class __T, class __ST>
    class ReadLineFunc {
        public:
        virtual Record<__T, __ST> operator() (const std::string& __str) const = 0;
    };

    /// @brief 实现的基本行读取函数对象，适用于类csv格式
    /// @tparam __T 数据类型
    /// @tparam __ST 标签类型
    template<class __T, class __ST>
    class DefaultReadLine : public ReadLineFunc<__T, __ST> {
        public:
        /// @brief 初始化函数对象
        /// @param __sep 分隔符
        /// @param __dimension 特征数
        DefaultReadLine(char __sep, long long __dimension):
        sep(__sep), dimension(__dimension) {}

        Record<__T, __ST> operator() (const std::string& __str) const override {
            std::vector<std::string> temp;
            std::vector<__T> __vec;
            __T vec_data; __ST label;
            splitString(__str, temp, sep);
#if __cplusplus >= 201402L && __cpp_lib_to_chars >= 201611L
            for (long long i = 0; i < dimension; ++i) {
                std::from_chars(temp[i].c_str(), temp[i].c_str() + temp[i].size(), vec_data);
                __vec.push_back(vec_data);
            }
            if constexpr (std::is_integral_v<__ST> || std::is_floating_point_v<__ST>) {
                std::from_chars(temp[dimension].c_str(), temp[dimension].c_str() + temp[dimension].size(), label);
            } else {
                label = temp[dimension];
            }
#else
            for (long long i = 0; i < dimension; ++i) {
                fromStr(temp[i], vec_data);
                __vec.push_back(vec_data);
            }
            fromStr(temp[dimension], label);
#endif
            return Record<__T, __ST>{__vec, label};
        }

        private:
        char sep;
        long long dimension;
    };

    template<class __T, class __ST>
    /// @brief 从文件中读入数据集
    /// @tparam __T 向量数据类型
    /// @tparam __ST 标签数据类型
    /// @param __filename 文件名
    /// @param __ds_ref 数据集的引用
    /// @param __line_func 对于每一行执行的函数
    void readDatasetFile(const char* __filename, DataSet<__T, __ST>& __ds_ref, 
                        std::function<Record<__T, __ST>(const std::string&)> __line_func,
                        int __skipped = 0) {
        std::ifstream ifs(__filename, std::ios::in);
        if (ifs.is_open()) {
            std::string __buf;
            while (std::getline(ifs, __buf)) {
                if (__skipped) {
                    --__skipped;
                    continue;
                }
                if (__buf.size()) __ds_ref.appendRecord(__line_func(__buf));
            }
            ifs.close();
        }
    }

    template<class __T, class __ST>
    /// @brief 从文件中读入数据集（针对函数对象重载）
    /// @tparam __T 数据类型
    /// @tparam __ST 标签类型
    /// @param __filename 文件名
    /// @param __ds_ref 数据集的引用
    /// @param __line_func_obj 函数对象 (`ReadLineFunc`的子类)
    void readDatasetFile(const char* __filename, DataSet<__T, __ST>& __ds_ref, 
                        const ReadLineFunc<__T, __ST>& __line_func_obj, int __skipped = 0) {
        std::ifstream ifs(__filename, std::ios::in);
        if (ifs.is_open()) {
            std::string __buf;
            while (std::getline(ifs, __buf)) {
                if (__skipped) {
                    --__skipped;
                    continue;
                }
                if (__buf.size()) __ds_ref.appendRecord(__line_func_obj(__buf));
            }
            ifs.close();
        }
    }

    template<class __T, class __ST>
    /// @brief 格式化打印结果，包括选取的数据信息以及标签的数量统计
    /// @tparam __T 向量数据类型
    /// @tparam __ST 标签数据类型
    /// @param __ret_vec 任意KNN对象的`get`方法返回的记录指针数组
    /// @param __detail_display 是否打印详细信息
    void collectResult(const std::vector<const Record<__T, __ST>*>& __ret_vec, 
                       bool __detail_display = true) {
        if (__detail_display) {
            std::cout << "----------------------------" << '\n';
            std::cout << "According to ascending order:\n";
        }
        std::unordered_map<__ST, int> collect;
        const Record<__T, __ST>* __rec;
        for (int i = 0; i < __ret_vec.size(); ++i) {
            __rec = __ret_vec.at(i);
            if (__detail_display) {
                std::cout << std::left;
                for (int j = 0; j < __rec->vec.size(); ++j) {
                    std::cout << std::setw(10) << __rec->vec[j];
                }
                std::cout << "  ->  " << __rec->state << '\n';
                std::cout.unsetf(std::ios::left);
            }
            auto it = collect.find(__rec->state);
            if (it != collect.end()) {
                it->second += 1;
            } else {
                collect[__rec->state] = 1;
            }
        }
        std::cout << "----------------------------" << '\n';
        std::cout << "Summary:" << '\n';
        std::cout << "Label : Frequency | Percentage" << '\n';
        std::cout << std::left;
        for (auto it = collect.begin(); it != collect.end(); ++it) {
            std::cout << std::setw(5) << it->first << " : " 
                    << std::setw(9) << it->second << " | "
                    << (static_cast<double>(it->second) / __ret_vec.size()) << '\n';
        }
        std::cout.unsetf(std::ios::left);
        std::cout << "----------------------------" << '\n';
        std::flush(std::cout);
    }

    template<class __T, class __ST>
    /// @brief 由指定数据集创建训练集和测试集
    /// @tparam __T 数据类型
    /// @tparam __ST 标签类型
    /// @param __source 源数据集
    /// @param __training_group 指定的训练集
    /// @param __test_group 指定的测试集
    /// @param __test_size 测试集大小
    void selectTestGroup(const DataSet<__T, __ST>& __source, DataSet<__T, __ST>& __training_group, 
                         DataSet<__T, __ST>& __test_group, long long __test_size) {
        if (__test_size < 0) return ;
        if (__test_size > __source.dataSize()) return;

        std::set<long long> check_map;
        std::random_device r;
        std::default_random_engine rand_engine(r());
        std::uniform_int_distribution<long long> distribute(0, __source.dataSize() - 1);
        
        long long x;
        while (__test_size) {
            x = distribute(rand_engine);
            if (check_map.find(x) == check_map.end()) {
                check_map.insert(x);
                __test_group.appendRecord(*(__source.getRef(x)));
                --__test_size;
            }
        }
        for (long long i = 0; i < __source.dataSize(); ++i) {
            if (check_map.find(i) == check_map.end()) {
                __training_group.appendRecord(*(__source.getRef(i)));
            }
        }
    }
    
    template<class __T, class __DT, class __ST>
    /// @brief 检查k取特定值并固定测试集时预测的正确率
    /// @tparam __T 数据类型
    /// @tparam __DT 距离类型
    /// @tparam __ST 标签类型
    /// @param __knn `BaseKNN`对象
    /// @param __test_k k值
    /// @param __test_set 测试集
    /// @param thread_cnt 多线程查询线程数，若为非正数，则不使用多线程
    /// @return 正确率
    double testCorrectness(BaseKNN<__T, __DT, __ST>& __knn, int __test_k,
                         const DataSet<__T, __ST>& __test_set, int thread_cnt = -1) {
        long long correct = 0;
        std::vector<const Record<__T, __ST>*> results;
        std::unordered_map<__ST, int> collect;
        
        for (int i = 0; i < __test_set.dataSize(); ++i) {
            results.clear();  collect.clear();
            const Record<__T, __ST>* ptr = __test_set.getRef(i);
            if (thread_cnt <= 0) __knn.get(ptr->vec, __test_k, results);
            else __knn.multiThreadGet(ptr->vec, __test_k, thread_cnt, results);
            for (auto it = results.begin(); it != results.end(); ++it) {
                auto mit = collect.find((*it)->state);
                if (mit == collect.end()) collect[(*it)->state] = 1;
                else mit->second += 1;
            }
            int __max = INT_MIN; __ST label;
            for (auto it = collect.begin(); it != collect.end(); ++it) {
                if (it->second > __max) {
                    __max = it->second;
                    label = it->first;
                }
            }
            correct += (label == ptr->state) ? 1 : 0;
        }
        return static_cast<double>(correct) / __test_set.dataSize();
    }

    template<class __T, class __ST, class __KNN>
    double crossValidation(const DataSet<__T, __ST>& __dataset, int __k, int __group_cnt) {
        if (__group_cnt < 1) return 0.0;
        int tot_size = __dataset.dataSize();
        int dimension = __dataset.getDimension();
        int group_unit = tot_size / __group_cnt;
        int final_group = group_unit + (tot_size % __group_cnt);
        std::vector<const Record<__T, __ST>*> all_data;
        all_data.reserve(tot_size);
        for (int i = 0; i < tot_size; ++i) {
            all_data.push_back(__dataset.getRef(i));
        }
        std::random_device rd;
        std::default_random_engine gen_rand(rd());
        std::shuffle(all_data.begin(), all_data.end(), gen_rand);
        std::vector<std::pair<int, int>> group_ranges;
        group_ranges.reserve(__group_cnt);
        for (int i = 0; i < __group_cnt - 1; ++i) {
            group_ranges.push_back(std::make_pair(i * group_unit, (i + 1) * group_unit));
        }
        group_ranges.push_back(std::make_pair(tot_size - final_group, tot_size));
        
        // Start cv
        double acc_sum = 0.0;
        for (int i = 0; i < __group_cnt; ++i) {
            int group_size = group_ranges[i].second - group_ranges[i].first;
            DefaultDataSet<__T, __ST> train_set(dimension, tot_size - group_size), 
                                      test_set(dimension, group_size);
            for (int j = 0; j < tot_size; ++j) {
                if (j >= group_ranges[i].first && j < group_ranges[i].second) {
                    test_set.appendRecord(*all_data[j]);
                } else {
                    train_set.appendRecord(*all_data[j]);
                }
            }
            __KNN __knn_obj(train_set);
            acc_sum += testCorrectness(__knn_obj, __k, test_set);
        }
        return acc_sum / static_cast<double>(__group_cnt);
    }

    template<class __T, class __ST, class __KNN>
    /// @brief 获取范围内K的准确度
    /// @tparam __T 数据类型
    /// @tparam __ST 标签类型
    /// @tparam __KNN KNN类型
    /// @param __data_set 数据集
    /// @param iteration_cnt 重复次数
    /// @param __k_range 表示k范围的std::pair
    /// @param __group_count 交叉验证组数
    /// @param __container 结果容器，类型为`knn_k_optimization_ret_list`
    void kRangedCheck(const DataSet<__T, __ST>& data_set, int iteration_cnt,
                   std::pair<int, int> __k_range, int __group_count,
                   knn_ranged_k_ret_list& __container) {
        int __lower = __k_range.first;
        int __upper = __k_range.second;
        if (__lower > __upper) std::swap(__lower, __upper);
        __container.clear();
        std::unordered_map<int, double> k_map;

        for (int iter = 0; iter < iteration_cnt; ++iter) {
            for (int k = __lower; k <= __upper; ++k) {
                double c = crossValidation<__T, __ST, __KNN>(data_set, k, __group_count);
                auto it = k_map.find(k);
                if (it == k_map.end()) k_map.insert(std::make_pair(k, c));
                else it->second += c;
            }
        }

        for (auto it = k_map.begin(); it != k_map.end(); ++it) {
            it->second /= static_cast<double>(iteration_cnt);
            __container.push_back(*it);
        }
    }

    template<class __T, class __ST, class __KNN>
    void __optimize_sub_task(const DataSet<__T, __ST>* __ds, int iter, int __lk, int __uk,
                             int __cv, std::promise<std::unordered_map<int, double>*> __p) {
        auto map_ptr = new std::unordered_map<int, double>();

        for (int i = 0; i < iter; ++i) {
            for (int k = __lk; k <= __uk; ++k) {
                double c = crossValidation<__T, __ST, __KNN>(*__ds, k, __cv);
                auto it = map_ptr->find(k);
                if (it == map_ptr->end()) map_ptr->insert(std::make_pair(k, c));
                else it->second += c;
            }
        }
        __p.set_value(map_ptr);
    }

    template<class __T, class __ST, class __KNN>
    /// @brief 获取范围内K的准确度（多线程加速重载）
    /// @tparam __T 数据类型
    /// @tparam __ST 标签类型
    /// @tparam __KNN KNN类型
    /// @param __data_set 数据集
    /// @param iteration_cnt 重复次数
    /// @param thread_cnt 线程数
    /// @param __k_range 表示k范围的std::pair
    /// @param __group_count 交叉验证组数
    /// @param __container 结果容器，类型为`knn_k_optimization_ret_list`
    void kRangedCheck(const DataSet<__T, __ST>& data_set, int iteration_cnt, int thread_cnt,
                       std::pair<int, int> __k_range, int __group_count,
                       knn_ranged_k_ret_list& __container) {
        int __lower = __k_range.first;
        int __upper = __k_range.second;
        if (__lower > __upper) std::swap(__lower, __upper);
        __container.clear();
        std::unordered_map<int, double> tot_k_map;
        std::vector<std::thread*> thread_pool;
        std::vector<std::future<std::unordered_map<int, double>*>> thread_rets;
        thread_pool.reserve(thread_cnt);
        thread_rets.reserve(thread_cnt);

        if (iteration_cnt < thread_cnt) {
            kRangedCheck<__T, __ST, __KNN>(data_set, iteration_cnt, {__lower, 
                                           __upper}, __group_count, __container);
            return ;
        }

        int left = 0, right = 0, cnt = 0;
        int iteration_unit = iteration_cnt / thread_cnt;
        int final_unit = iteration_cnt - (iteration_unit * thread_cnt);
        for (int it = 0; it < thread_cnt; ++it) {
            if (it == thread_cnt - 1) cnt = final_unit + iteration_unit;
            else cnt = iteration_unit;
            std::promise<std::unordered_map<int, double>*> __pro;
            thread_rets.push_back(__pro.get_future());
            auto t_ptr = new std::thread(__optimize_sub_task<__T, __ST, __KNN>, 
                                         &data_set, cnt, __lower, __upper, __group_count, std::move(__pro));
            thread_pool.push_back(t_ptr);
        }

        for (int it = 0; it < thread_cnt; ++it) {
            auto t_ptr = thread_pool.at(it);
            t_ptr->join();
            std::unordered_map<int, double>* map_ptr = thread_rets[it].get();
            for (auto it = map_ptr->begin(); it != map_ptr->end(); ++it) {
                auto tot_it = tot_k_map.find(it->first);
                if (tot_it == tot_k_map.end()) tot_k_map.insert(*it);
                else tot_it->second += it->second;
            }
            delete t_ptr;
            delete map_ptr;
            thread_pool[it] = nullptr;
        }

        for (auto it = tot_k_map.begin(); it != tot_k_map.end(); ++it) {
            it->second /= static_cast<double>(iteration_cnt);
            __container.push_back(*it);
        }
    }

    void collectKDetails(knn_ranged_k_ret_list& __list, int height = 10,
                         bool show_diagram = false) {
        double __top = -1.0, __top_k = 0;
        double __bottom = 2.0;
        std::for_each(__list.begin(), __list.end(),
        [&__top, &__bottom, &__top_k] (const std::pair<int, double>& element) {
            if (__top < element.second) {
                __top = element.second;
                __top_k = element.first;
            }
            __bottom = std::min(__bottom, element.second);
        });
        double unit = (__top - __bottom) / height;
        std::cout << "Max accuracy: " << __top
                << " when k=" << __top_k << '\n';
        if (!show_diagram) return ;
        std::cout << "Full diagram:\n";
        std::sort(__list.begin(), __list.end(),
        [] (const std::pair<int, double>& left,
            const std::pair<int, double>& right) {
            return left.first < right.first;
        });
        
        std::vector<int> heights(__list.size(), 0);
        for (int i = 0; i < __list.size(); ++i) {
            heights[i] = round(((__list[i].second - __bottom) / (__top - __bottom)) * height);
        }
        std::cout << std::fixed << std::left << std::setprecision(4);
        for (int i = height; i >= 0; --i) {
            std::cout << std::setw(4) << __bottom + unit * i;
            for (int j = 0; j < __list.size(); ++j) {
                if (heights[j] == i) std::cout << 'x';
                else std::cout << ' ';
            }
            std::cout << '\n';
        }
        std::cout.unsetf(std::ios::fixed | std::ios::left);
        std::cout << std::setprecision(6);
        std::cout << "K range: [" << __list[0].first
                << ',' << __list[__list.size() - 1].first << "]\n";
    }

} /* namespace knn */

#endif

#endif /* DS_KNN_HPP */