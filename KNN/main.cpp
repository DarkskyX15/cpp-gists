/*
 * @Date: 2024-03-28 20:53:03
 * @Author: DarkskyX15
 * @LastEditTime: 2024-05-31 15:54:07
 */
#include "knn.hpp"
#include "config.hpp"

using namespace knn;

int executed_cnt = 0;
int global_thread_cnt, global_max_line, global_diag_height;
bool global_detail_print, global_range_diag;
std::string global_allow_start, global_start_path;
std::unordered_map<std::string, std::pair<knn::BaseKNN<double, double, std::string>*, int>> knn_storage;
std::unordered_map<std::string, knn::DefaultDataSet<double, std::string>*> dataset_storage;
std::unordered_map<std::string, int> k_val_storage;
std::set<std::string> variable_table;
std::string run_id;

inline void showErr(const std::string& __cmd, const std::string& __err) {
    std::cout << "[Err] Error executing command: " << __cmd
                << "\n\t" << __err << '\n';
}

inline bool checkFile(const std::string& __source) {
    std::ifstream file_check(__source, std::ios::in);
    if (!file_check) return false;
    else {
        file_check.close();
        return true;
    }
}

void generateRunId() {
    auto time_tick = std::chrono::system_clock::now();
    std::stringstream output(std::ios::out);
    output << std::chrono::duration_cast<std::chrono::milliseconds>(time_tick.time_since_epoch()).count();
    run_id = output.str();
}

std::vector<std::string> readCommandFile(const std::string& __source) {
    std::ifstream cmd_in(__source, std::ios::in);
    if (!cmd_in) {
        std::cout << "[Err] Failed to read command file: " << __source << '\n';
        return {};
    }
    std::vector<std::string> cmd_lines;
    std::string read_buffer;
    while (std::getline(cmd_in, read_buffer)) {
        if (read_buffer.size() <= 0) continue;
        cmd_lines.push_back(read_buffer);
    }
    cmd_in.close();
    return cmd_lines;
}

bool operateDataset(const std::vector<std::string>& __args,
                    DefaultDataSet<double, std::string>* __target) {
    if (__args[1] == "z-score") {
        std::cout << "Perform z-score normalization on " << __args[0] << '\n';
        __target->zScoreNormalization();
        return true;
    }
    return false;
}

bool operateKNN(const std::vector<std::string>& __args,
                int knn_type, BaseKNN<double, double, std::string>* __target) {
    return false;
}

bool executeCommand(const std::string& __cmd) {
    executed_cnt += 1;
    std::vector<std::string> args;
    cfg::splitString(__cmd, args);
    if (args.size() <= 0) {
        showErr(__cmd, "Invalid length.");
        return false;
    } else {
        if (args[0] == "exit") {
            std::cout << "Deconstruct instances...\n";
            for (auto pair : dataset_storage) {
                delete pair.second;
            }
            for (auto pair : knn_storage) {
                delete pair.second.first;
            }
            std::cout << "Command caused exit.\n";
            exit(0);

        } else if (args[0] == "function") {
            // err
            if (args.size() < 2) {
                showErr(__cmd, "Missing command file name.");
                return false;
            }
            std::vector<std::string> cmd_lines(readCommandFile(args[1]));
            if (!cmd_lines.size()) return false;
            for (const auto& line : cmd_lines) {
                // err
                if ((executed_cnt > global_max_line) && global_max_line > 0) {
                    showErr(__cmd, "Command count limit exceeded!");
                    return false;
                }
                if (!executeCommand(line)) {
                    showErr(__cmd, "Stop command file execution due to thrown error");
                    return false;
                }
            }
            std::cout << "Finish command execution: " << args[1] << '\n';
            return true;

        } else if (args[0] == "k_val") {
            // err
            if (args.size() < 3) {
                showErr(__cmd, "Expected format: k_val <variable_name> <value>");
                return false;
            }
            auto iter = variable_table.find(args[1]);
            if (iter != variable_table.end()) {
                showErr(__cmd, "Redefined variable:" + args[1]);
                return false;
            }
            int k_val;
            fromStr(args[2], k_val);
            k_val_storage.insert(std::make_pair(args[1], k_val));
            variable_table.insert(args[1]);
            std::cout << "Stored k value: " << args[1] << '\n';
            return true;

        } else if (args[0] == "load") {
            // err
            if (args.size() < 2) {
                showErr(__cmd, "Expected format: load <save_name>");
                return false;
            }
            std::string func("function .\\saves\\" + args[1] + ".knn");
            std::cout << "Try load model with command: " << func << '\n';
            return executeCommand(func);

        } else if (args[0] == "save") {
            // err
            if (args.size() < 4) {
                showErr(__cmd, "Expected format: save <knn> <k> <save_name>");
                return false;
            }
            auto iter = knn_storage.find(args[1]);
            if (iter == knn_storage.end()) {
                showErr(__cmd, "Cannot find knn object: " + args[1]);
                return false;
            }
            // generate paths
            std::string dataset_path(".\\saves\\datasets\\" + args[3] + run_id + ".bin");
            std::string save_path(".\\saves\\" + args[3] + ".knn");
            std::string knn_type(iter->second.second == 1 ? "kd-tree" : "brute");

            auto knn_obj = iter->second.first;
            auto dataset_ptr = dynamic_cast<const DefaultDataSet<double, std::string>*>(knn_obj->getDatasetRef());
            dataset_ptr->saveToBin(dataset_path.c_str());
            std::ofstream knn_write(save_path, std::ios::out);
            knn_write << "dataset dataset_" << args[3] << " bin " << dataset_path << '\n';
            knn_write << "k_val k_" << args[3] << ' ' << args[2] << '\n';
            knn_write << "knn " << args[3] << ' ' << knn_type << " dataset_" << args[3] << '\n';
            knn_write.close();

            std::cout << "Successfully save model " << args[1] << " with name " << args[3] << '\n';
            return true;

        } else if (args[0] == "dataset") {
            // err
            if (args.size() < 4) {
                showErr(__cmd, "Expected format: dataset <variable_name> <type> <file_path> [sep]");
                return false;
            }
            char arg_sep = (args.size() >= 5 ? args[4][0] : -1);

            // name check
            auto it = variable_table.find(args[1]);
            if (it != variable_table.end()) {
                showErr(__cmd, "Redefined variable: " + args[1]);
                return false;
            }
            // file check
            if (!checkFile(args[3])) {
                showErr(__cmd, "Cannot read file:" + args[3]);
                return false;
            }
            // do bin first
            if (args[2] == "bin") {
                auto ds_ptr = new DefaultDataSet<double, std::string>();
                ds_ptr->loadFromBin(args[3].c_str());
                dataset_storage.insert(std::make_pair(args[1], ds_ptr));
                variable_table.insert(args[1]);
                std::cout << "Created dataset instance: " << args[1] << '\n';
                return true;
            }
            // check if raw
            if (args[2] != "raw") {
                showErr(__cmd, "Unknown file type: " + args[2]);
                return false;
            }
            // predict
            std::ifstream check_in(args[3], std::ios::in);
            std::string check_buf;
            std::vector<std::string> check_list;
            int skipped_lines = -1;
            do {
                ++skipped_lines;
                std::getline(check_in, check_buf);
                if (!check_in) break;
                if (!check_buf.size()) continue;
                bool valid = true;
                cfg::splitString(check_buf, check_list, arg_sep);
                for (int index = 0; index < check_list.size() - 1 && valid; ++index) {
                    for (char chr : check_list[index]) {
                        if (!((chr >= '0' && chr <= '9') || chr == '.')) {
                            valid = false;
                            break;
                        }
                    }
                }
                if (!valid) continue;
                else break;
            } while (true);
            check_in.close();
            
            std::cout << "Skipped lines: " << skipped_lines << '\n';
            std::cout << "Predict line: " << check_buf << '\n';
            int dimension = check_list.size() - 1;
            std::cout << "Predict dimension: " << dimension << '\n';

            auto ds_ptr = new DefaultDataSet<double, std::string>(dimension);
            readDatasetFile(args[3].c_str(), *ds_ptr,
                            DefaultReadLine<double, std::string>(arg_sep, dimension), skipped_lines);
            dataset_storage.insert(std::make_pair(args[1], ds_ptr));
            variable_table.insert(args[1]);

            std::cout << "Created dataset instance: " << args[1] << '\n';
            return true;

        } else if (args[0] == "knn") {
            // err
            if (args.size() < 4) {
                showErr(__cmd, "Expected format: knn <variable_name> <structure> <dataset>"
                        "\n\t structure can only be 'brute' or 'kd-tree'");
                return false;
            }

            // check name & dataset
            auto sit = variable_table.find(args[1]);
            if (sit != variable_table.end()) {
                showErr(__cmd, "Redefined variable: " + args[1]);
                return false;
            }
            auto dit = dataset_storage.find(args[3]);
            if (dit == dataset_storage.end()) {
                showErr(__cmd, "Cannot find dataset instance: " + args[3]);
                return false;
            }
            
            // type diff
            if (args[2] == "kd-tree") {
                auto knn_ptr = new KDTree<double, double, std::string>(*(dit->second));
                auto base_ptr = dynamic_cast<BaseKNN<double, double, std::string>*>(knn_ptr);
                knn_storage.insert(std::make_pair(args[1], std::make_pair(base_ptr, 1)));
                variable_table.insert(args[1]);
                std::cout << "Created KNN instance " << args[1] << " with structure KD-Tree at " << base_ptr << '\n';
                return true;
            } else if (args[2] == "brute") {
                auto knn_ptr = new Brute<double, double, std::string>(*(dit->second));
                auto base_ptr = dynamic_cast<BaseKNN<double, double, std::string>*>(knn_ptr);
                knn_storage.insert(std::make_pair(args[1], std::make_pair(base_ptr, 0)));
                variable_table.insert(args[1]);
                std::cout << "Created KNN instance " << args[1] << " with structure Brute at " << base_ptr << '\n';
                return true;
            } else {
                showErr(__cmd, "Unknown structure: " + args[2]);
                return false;
            }

        } else if (args[0] == "cv") {
            // err
            if (args.size() < 5) {
                showErr(__cmd, "Expected format: cv <dataset> <brute/kd-tree> <k> <group_cnt>");
                return false;
            }
            // check dataset
            auto dit = dataset_storage.find(args[1]);
            if (dit == dataset_storage.end()) {
                showErr(__cmd, "Cannot find dataset object: " + args[1]);
                return false;
            }
            // diff mode
            double ans = 0.0;
            int k; fromStr(args[3], k);
            int groups; fromStr(args[4], groups);
            if (args[2] == "brute") {
                ans = crossValidation<double, std::string, Brute<double, double, std::string>>(*(dit->second), k, groups);
            } else if (args[2] == "kd-tree") {
                ans = crossValidation<double, std::string, KDTree<double, double, std::string>>(*(dit->second), k, groups);
            } else {
                showErr(__cmd, "Unknown knn structure: " + args[2]);
                return false;
            }
            std::cout << "Cross validation result with k=" << k
                    << ", group=" << groups << " : " << ans << '\n';
            return true;
        } 
        else if (args[0] == "range") {
            if (args.size() < 7) {
                showErr(__cmd, "Expected format: range <begin> <end> <iteration> <group_cnt> <dataset> <brute/kd-tree>");
                return false;
            }
            // check range
            int k_begin; fromStr(args[1], k_begin);
            int k_end; fromStr(args[2], k_end);
            if (k_begin > k_end) std::swap(k_begin, k_end);
            int iterations; fromStr(args[3], iterations);
            int groups; fromStr(args[4], groups);
            if (k_begin < 0 || k_end < 0) {
                showErr(__cmd, "Negative range");
                return false;
            }
            if (iterations <= 0 || groups <= 0) {
                showErr(__cmd, "Invalid cross validation args");
                return false;
            }
            // check dataset
            auto dit = dataset_storage.find(args[5]);
            if (dit == dataset_storage.end()) {
                showErr(__cmd, "Cannot find dataset object: " + args[5]);
                return false;
            }
            // diff mode
            std::cout << "Start ranged k check from " << k_begin << " to "
                    << k_end << "\nIteration count: " << iterations << " Use threads: "
                    << global_thread_cnt << "\nUse diagram: " << global_range_diag
                    << " Group count: " << groups << '\n';
            knn_ranged_k_ret_list answers;
            if (args[6] == "brute") {
                if (global_thread_cnt > 0) {
                    kRangedCheck<double, std::string, Brute<double, double, std::string>>
                    (*(dit->second), iterations, global_thread_cnt, {k_begin, k_end}, groups, answers);
                } else {
                    kRangedCheck<double, std::string, Brute<double, double, std::string>>
                    (*(dit->second), iterations, {k_begin, k_end}, groups, answers);
                }
            } else if (args[6] == "kd-tree") {
                if (global_thread_cnt > 0) {
                    kRangedCheck<double, std::string, KDTree<double, double, std::string>>
                    (*(dit->second), iterations, global_thread_cnt, {k_begin, k_end}, groups, answers);
                } else {
                    kRangedCheck<double, std::string, KDTree<double, double, std::string>>
                    (*(dit->second), iterations, {k_begin, k_end}, groups, answers);
                }
            } else {
                showErr(__cmd, "Unknown knn structure: " + args[6]);
                return false;
            }
            collectKDetails(answers, global_diag_height, global_range_diag);
            return true;

        } else if (args[0] == "predict") {
            // err
            if (args.size() < 4) {
                showErr(__cmd, "Expected format: predict <knn> [k] <direct/file> {vec}/<path>");
                return false;
            }
            int data_source_pos = 0;
            for (int i = 0; i < args.size(); ++i) {
                if (args[i] == "direct" || args[i] == "file") {
                    data_source_pos = i;
                    break;
                }
            }
            // get args
            std::string knn_name, data_source, k_str_val;
            int direct_start = 0;
            if (data_source_pos == 3) {
                // with k
                knn_name = args[1];
                k_str_val = args[2];
                data_source = args[3];
                direct_start = 4;
            } else if (data_source_pos == 2) {
                // without k
                knn_name = args[1];
                k_str_val = "-1";
                data_source = args[2];
                direct_start = 3;
            } else {
                showErr(__cmd, "Expected format: predict <knn> [k] <direct/file> <vec>/<path>");
                return false;
            }
            // check knn
            auto kit = knn_storage.find(knn_name);
            if (kit == knn_storage.end()) {
                showErr(__cmd, "Cannot find knn object: " + knn_name);
                return false;
            }
            // read to predict
            std::vector<std::vector<double>> wait_query;
            if (data_source == "file") {
                if (!checkFile(args[direct_start])) {
                    showErr(__cmd, "Cannot open file: " + args[direct_start]);
                    return false;
                }
                std::ifstream test_in(args[direct_start], std::ios::in);
                std::string line_temp;
                std::vector<std::string> temp_split;
                std::vector<double> temp_test;
                double x;
                while (std::getline(test_in, line_temp)) {
                    if (line_temp.size() <= 0) continue;
                    temp_test.clear();
                    cfg::splitString(line_temp, temp_split);
                    for (auto& sp : temp_split) {
                        fromStr(sp, x);
                        temp_test.push_back(x);
                    }
                    wait_query.push_back(temp_test);
                }
                test_in.close();
            } else if (data_source == "direct") {
                std::vector<double> temp_test;
                double x;
                for (int i = direct_start; i < args.size(); ++i) {
                    fromStr(args[i], x);
                    temp_test.push_back(x);
                }
                wait_query.push_back(temp_test);
            } else {
                showErr(__cmd, "Unknown prediction source: " + data_source);
                return false;
            }
            // check multi
            int k, idx = 0; fromStr(k_str_val, k);
            if (k <= 0) {
                auto iter = k_val_storage.find("k_" + knn_name);
                if (iter == k_val_storage.end()) {
                    showErr(__cmd, "Cannot find stored default k value for: " + knn_name);
                    return false;
                } else {
                    k = iter->second;
                }
            }
            bool multi_flg = (global_thread_cnt > 0) ? true : false;
            std::cout << "Start prediction with k=" << k << "\nMultithread: "
                    << (multi_flg ? "Enable " : "Disable ") << " Total: "
                    << wait_query.size() << '\n';
            // start predict
            auto dataset = kit->second.first->getDatasetRef();
            for (auto& vec : wait_query) {
                ++idx;
                std::cout << "Prediction " << idx << " -> ";
                for (auto& dat : vec) {
                    std::cout << dat << ' ';
                } std::cout << " :\n";

                auto result = kit->second.first->getResultContainer();
                auto temp_sync = dataset->syncNormalization(vec);
                if (multi_flg) kit->second.first->multiThreadGet(temp_sync, k, global_thread_cnt, result);
                else kit->second.first->get(temp_sync, k, result);
                collectResult(result, global_detail_print);
            }
            std::cout << "Prediction finished.\n";
            return true;
            
        } else if (args[0] == "variables") {
            std::cout << "Created values:\n\nDataset objects:\n";
            for (auto it : dataset_storage) {
                std::cout << it.first << " at " << it.second << '\n';
            }
            std::cout << "\nKNN objects:\n";
            for (auto it : knn_storage) {
                std::cout << it.first << " at " << it.second.first << " structure: ";
                if (it.second.second == 1) std::cout << "kd-tree\n";
                else std::cout << "brute\n";
            }
            std::cout << "\nStored K values:\n";
            for (auto it : k_val_storage) {
                std::cout << it.first << ':' << it.second << '\n';
            }
            return true;
        } else if (args[0] == "help") {
            std::cout <<
                "启动时可附加参数，该参数为需要执行的命令文件路径。" 
                "可用的命令：\n"
                "\ndataset -> 创建数据集对象\n\t"
                "格式: dataset <变量名> <文件类型> <文件路径> [分隔符]\n\t"
                "文件类型只能在raw和bin中选其一。\n\t"
                "若选择raw，则将数据集每行按分隔符分隔，若省略则按空白字符分隔。数据维数自动检测。\n\t"
                "若选择bin，则按KNN.hpp中设置的模式读取以二进制保存的数据集。此时分隔符参数被忽略。\n"
                "\nknn -> 创建KNN对象\n\t"
                "格式: knn <变量名> <计算方法> <绑定数据集>\n\t"
                "绑定数据集应为已经创建了的数据集对象的变量名。\n\t"
                "计算方法参数只能'brute'和'kd-tree'选其一。\n"
                "\npredict -> 用指定KNN对象预测未知数据\n\t"
                "格式: predict <KNN对象名> [k] <数据来源> {数据}/<文件路径>\n\t"
                "数据来源参数只能'file'和'direct'选其一\n\t"
                "选择file应提供命令最后的文件路径参数，该文本文件每行包含一个需要预测的数据。\n\t"
                "选择direct应提供数据参数，数据参数为需要预测的向量，每个特征以空格分隔。\n\t"
                "若未指定k，则尝试从k储存中选取名为\"k_{KNN对象名}\"的数据执行命令。\n\t"
                "配置文件中useDetailedPrint选项控制是否详细按距离升序输出k近邻。\n\t"
                "配置文件中multiThreadCount选项控制使用的线程数。\n"
                "\nvariables -> 显示已创建的数据集和KNN对象\n\t"
                "格式: variables\n"
                "\ncv -> 对指定数据集进行关于k的交叉验证\n\t"
                "格式: cv <数据集> <计算方法> <k> <分组数量>\n\t"
                "计算方法参数只能'brute'和'kd-tree'选其一。\n"
                "\nrange -> 对区间内的k批量交叉验证并统计输出\n\t"
                "格式: range <开始k> <结束k> <重复次数> <分组数量> <数据集> <计算方法>\n\t"
                "计算方法参数只能'brute'和'kd-tree'选其一。\n\t"
                "配置文件中useRangedDiagram选项控制统计输出是否启用图表\n\t"
                "配置文件中diagramHeight选项控制图表高度\n"
                "\nfunction -> 执行命令文件\n\t"
                "格式: function <命令文件路径>\n\t"
                "配置文件中maxLinePerCommand选项控制一次执行的最大命令数, <=0则不做限制\n"
                "\n<变量名标识符> -> 对创建的对象进行操作\n\t"
                "格式: <变量名标识符> [参数]\n\t"
                "若省略参数则显示对象的内存位置并提供可用参数的说明。\n"
                "\nk_val -> 创建储存k参数的变量\n\t"
                "格式: k_val <变量名标识符> <值>\n\t"
                "储存中名为\"k_{KNN对象名}\"的k值将在对对应KNN对象执行predict命令时被用于替换非正的k值。\n"
                "\nsave -> 保存KNN对象及其链接的数据集\n\t"
                "格式: save <KNN对象名> <k> <组合名称>\n\t"
                "将已创建的KNN对象与K参数和数据集组合保存。\n\t"
                "保存后的文件在 .\\saves 中，链接的数据集以二进制形式保存于 .\\saves\\datasets 中。\n"
                "\nload -> 加载保存的KNN对象\n\t"
                "格式: load <组合名称>\n\t"
                "加载模型将会创建名为\"dataset_{组合名称}\"的数据集，名为\"{组合名称}\"的KNN对象，\n\t"
                "以及名为\"k_{组合名称}\"的k值储存。\n"
                "\nexit -> 退出并释放创建的对象内存\n\t"
                "格式: exit\n";
            return true;
        } else {
            auto it = variable_table.find(args[0]);
            // first find
            int var_type = 0;
            if (it == variable_table.end()) {
                showErr(__cmd, "Unknown identifier: " + args[0]);
                return false;
            } else {
                auto it = dataset_storage.find(args[0]);
                if (it != dataset_storage.end()) {
                    var_type = 1; // dataset
                } else {
                    var_type = 2; // knn
                }
            }
            // second find
            if (var_type == 1) {
                auto it = dataset_storage.find(args[0]);
                if (args.size() == 1) {
                    std::cout << "Dataset object " << args[0]
                            << " at " << (void*)(it->second) << '\n';
                    std::cout << "Available args: z-score\n";
                    return true;
                } else {
                    bool ret = operateDataset(args, it->second);
                    if (!ret) showErr(__cmd, "Unknown arg for dataset operation.");
                    return ret;
                }
            } else if (var_type == 2) {
                auto it = knn_storage.find(args[0]);
                if (args.size() == 1) {
                    std::cout << "KNN object " << args[0]
                            << " at " << (void*)(it->second.first) << '\n';
                    std::cout << "Available args:\n";
                    return true;
                } else {
                    bool ret = operateKNN(args, it->second.second, it->second.first);
                    if (!ret) showErr(__cmd, "Unknown arg for knn operation");
                    return ret;
                }
            }
        }
    }
    showErr(__cmd, "Unknown error");
    return false;
}

int main(int argc, char* argv[]) {

    generateRunId();

    // Read Config file
    cfg::Config global_cfg;
    std::ifstream test_file(".\\config.txt", std::ios::in);
    if (!test_file) {
        std::cout << "Config does not exist!\n";
        std::cout << "Try create default config...\n";
        std::ofstream out_file(".\\config.txt", std::ios::out);
        if (!out_file) {
            std::cout << "Fail to create default config\n";
            return -1;
        }
        out_file << "# 启用开始时命令\n"
                << "allowStartCommand=false\n"
                << "# 每次执行包含的最多命令数\n"
                << "maxLinePerCommand=-1\n"
                << "# 开始时命令的路径\n"
                << "startCommandFile=path/of/command_file\n"
                << "# 多线程数，小于等于0时不使用多线程\n"
                << "multiThreadCount=20\n"
                << "# 启用细节打印\n"
                << "useDetailedPrint=true\n"
                << "# 启用K图表绘制\n"
                << "useRangedDiagram=true\n"
                << "# 图表高度\n"
                << "diagramHeight=10\n"
                << "# 在Windows上将编码调整为Unicode\n"
                << "windowsUnicode=true\n"
                << "# 不进入交互模式\n"
                << "noInteraction=false\n";
        out_file.close();
        std::cout << "Created default config!\n";
    } else {
        test_file.close();
    }
    global_cfg.readFromFile(".\\config.txt");
    if (!global_cfg) {
        std::cout << "[Err] Failed to load config:\n\t"
                << global_cfg.get_fail_msg() << '\n';
        return -1;
    }
    global_cfg.get_helper("allowStartCommand", global_allow_start);
    global_cfg.get_helper("maxLinePerCommand", global_max_line);
    global_cfg.get_helper("multiThreadCount", global_thread_cnt);
    global_cfg.get_helper("startCommandFile", global_start_path);
    std::string flg_temp;
    global_cfg.get_helper("useDetailedPrint", flg_temp);
    global_detail_print = (flg_temp == "true") ? true : false;
    global_cfg.get_helper("useRangedDiagram", flg_temp);
    global_range_diag = (flg_temp == "true") ? true : false;
    bool no_interact;
    global_cfg.get_helper("noInteraction", flg_temp);
    no_interact = (flg_temp == "true") ? true : false;
    global_cfg.get_helper("diagramHeight", global_diag_height);
    std::string win_unicode;
    global_cfg.get_helper("windowsUnicode", win_unicode);
    if (win_unicode == "true") system("chcp 65001");


    std::cout << "Launch with config:\n"
                "allowStartCommand: " << global_allow_start << '\n'
            << "maxLinePerCommand: " << global_max_line << '\n'
            << "multiThreadCount: " << global_thread_cnt << '\n'
            << "startCommandFile: " << global_start_path << '\n'
            << "useRangedDiagram: " << (global_range_diag ? "true" : "false") << '\n'
            << "useDetailedPrint: " << (global_detail_print ? "true" : "false") << '\n'
            << "noInteraction: " << (no_interact ? "true" : "false") << '\n'
            << "diagramHeight: " << global_diag_height << '\n'
            << "windowsUnicode: " << win_unicode << "\n\n";

    std::cout << "Run ID: " << run_id << "\n\n";

    // PreStart
    if (global_allow_start == "true") {
        std::cout << "Start command enabled: " << global_start_path << '\n';
        std::cout << "Try to run command...\n";
        std::vector<std::string> cmd_lines(readCommandFile(global_start_path));
        if (cmd_lines.size()) {
            bool result = true;
            for (const auto& elem : cmd_lines) {
                result = executeCommand(elem);
                if (!result) {
                    std::cout << "[Err] Start command execution failed\n";
                    break;
                }
            }
        }
        std::cout << "Start command execution finished.\n";
    } else {
        std::cout << "No start command.\n";
    }

    // Check Console Args
    if (argc <= 1) {
        std::cout << "No console args found.\n";   
    } else {
        std::cout << "Get command file path: " << argv[1] << '\n';
        std::cout << "Try open...\n";
        std::vector<std::string> cmd_lines(readCommandFile(argv[1]));
        if (cmd_lines.size()) {
            bool result = true;
            for (const auto& elem : cmd_lines) {
                result = executeCommand(elem);
                if (!result) {
                    std::cout << "Failed to execute command file: \n\t"
                            << "Path: " << argv[1] << '\n';
                    break;
                }
            }
        }
        std::cout << "Command file execution finished.\n";
    }

    // Start command mode
    if (no_interact) {
        std::cout << "Skip command mode due to config settings.\n";
        return 0;
    } else {
        std::cout << "Start command mode.\n";
    }
    std::cout << "\nType 'help' for instructions.\n";
    std::string cmd_buffer;
    while (true) {
        std::cout << ">>> " << std::flush;
        std::getline(std::cin, cmd_buffer);
        bool ret = executeCommand(cmd_buffer);
        if (ret) std::cout << "-> Success!\n\n";
        else std::cout << "-> Failed...\n\n";
    }
    return 0;
}