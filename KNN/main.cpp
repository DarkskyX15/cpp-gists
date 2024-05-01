/*
 * @Date: 2024-03-28 20:53:03
 * @Author: DarkskyX15
 * @LastEditTime: 2024-05-01 01:52:10
 */
#include "knn.hpp"
#include "config.hpp"

using namespace knn;

int executed_cnt = 0;
int global_thread_cnt, global_max_line, global_diag_height;
bool global_detail_print, global_range_diag;
std::string global_allow_start, global_start_path;
std::unordered_map<std::string, std::pair<knn::BaseKNN<double, double, int>*, int>> knn_storage;
std::unordered_map<std::string, knn::DefaultDataSet<double, int>*> dataset_storage;
std::set<std::string> variable_table;

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
                    DefaultDataSet<double, int>* __target) {
    if (__args[1] == "z-score") {
        std::cout << "Perform z-score normalization on " << __args[0] << '\n';
        __target->zScoreNormalization();
        return true;
    }
    return false;
}

bool operateKNN(const std::vector<std::string>& __args,
                int knn_type, BaseKNN<double, double, int>* __target) {
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

        } else if (args[0] == "dataset") {
            // err
            if (args.size() < 3) {
                showErr(__cmd, "Expected format: dataset <variable_name> <file_path> [sep]");
                return false;
            }
            char arg_sep = (args.size() >= 4 ? args[3][0] : -1);

            // name check
            auto it = variable_table.find(args[1]);
            if (it != variable_table.end()) {
                showErr(__cmd, "Redefined variable: " + args[1]);
                return false;
            }
            // file check & predict
            if (!checkFile(args[2])) {
                showErr(__cmd, "Cannot read file:" + args[2]);
                return false;
            }
            std::ifstream check_in(args[2], std::ios::in);
            std::string check_buf;
            do std::getline(check_in, check_buf);
            while (!check_buf.size());
            check_in.close();
            
            std::cout << "Predict line: " << check_buf << '\n';
            std::vector<std::string> check_list;
            cfg::splitString(check_buf, check_list, arg_sep);
            int dimension = check_list.size() - 1;
            std::cout << "Predict dimension: " << dimension << '\n';

            auto ds_ptr = new DefaultDataSet<double, int>(dimension);
            readDatasetFile(args[2].c_str(), *ds_ptr, DefaultReadLine<double, int>(arg_sep, dimension));
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
                auto knn_ptr = new KDTree<double, double, int>(*(dit->second));
                auto base_ptr = dynamic_cast<BaseKNN<double, double, int>*>(knn_ptr);
                knn_storage.insert(std::make_pair(args[1], std::make_pair(base_ptr, 1)));
                variable_table.insert(args[1]);
                std::cout << "Created KNN instance with structure KD-Tree at " << base_ptr << '\n';
                return true;
            } else if (args[2] == "brute") {
                auto knn_ptr = new Brute<double, double, int>(*(dit->second));
                auto base_ptr = dynamic_cast<BaseKNN<double, double, int>*>(knn_ptr);
                knn_storage.insert(std::make_pair(args[1], std::make_pair(base_ptr, 0)));
                variable_table.insert(args[1]);
                std::cout << "Created KNN instance with structure Brute at " << base_ptr << '\n';
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
                ans = crossValidation<double, int, Brute<>>(*(dit->second), k, groups);
            } else if (args[2] == "kd-tree") {
                ans = crossValidation<double, int, KDTree<>>(*(dit->second), k, groups);
            } else {
                showErr(__cmd, "Unknown knn structure: " + args[2]);
                return false;
            }
            std::cout << "Cross validation result with k=" << k
                    << ", group=" << groups << " : " << ans << '\n';
            return true;
        } 
        else if (args[0] == "range") {
            // TODO 更改优化方法 + 完成该部分命令
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
                    kRangedCheck<double, int, Brute<>>(*(dit->second), iterations, global_thread_cnt, {k_begin, k_end}, groups, answers);
                } else {
                    kRangedCheck<double, int, Brute<>>(*(dit->second), iterations, {k_begin, k_end}, groups, answers);
                }
            } else if (args[6] == "kd-tree") {
                if (global_thread_cnt > 0) {
                    kRangedCheck<double, int, KDTree<>>(*(dit->second), iterations, global_thread_cnt, {k_begin, k_end}, groups, answers);
                } else {
                    kRangedCheck<double, int, KDTree<>>(*(dit->second), iterations, {k_begin, k_end}, groups, answers);
                }
            } else {
                showErr(__cmd, "Unknown knn structure: " + args[6]);
                return false;
            }
            collectKDetails(answers, global_diag_height, global_range_diag);
            return true;

        } else if (args[0] == "predict") {
            // err
            if (args.size() < 6) {
                showErr(__cmd, "Expected format: predict <knn> <multithread> <k> <direct/file> <vec>/<path>");
                return false;
            }
            // check knn
            auto kit = knn_storage.find(args[1]);
            if (kit == knn_storage.end()) {
                showErr(__cmd, "Cannot find knn object: " + args[1]);
                return false;
            }
            // read to predict
            std::vector<std::vector<double>> wait_query;
            if (args[4] == "file") {
                if (!checkFile(args[5])) {
                    showErr(__cmd, "Cannot open file: " + args[4]);
                    return false;
                }
                std::ifstream test_in(args[5], std::ios::in);
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
            } else if (args[4] == "direct") {
                std::vector<double> temp_test;
                double x;
                for (int i = 5; i < args.size(); ++i) {
                    fromStr(args[i], x);
                    temp_test.push_back(x);
                }
                wait_query.push_back(temp_test);
            } else {
                showErr(__cmd, "Unknown command source: " + args[3]);
                return false;
            }
            // check multi
            int k, idx = 0; fromStr(args[3], k);
            bool multi_flg = (args[2] == "true") ? true : false;
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
            std::cout << "Created values:\nDataset objects:\n";
            for (auto it : dataset_storage) {
                std::cout << it.first << " at " << it.second << '\n';
            }
            std::cout << "KNN objects:\n";
            for (auto it : knn_storage) {
                std::cout << it.first << " at " << it.second.first << " structure: ";
                if (it.second.second == 1) std::cout << "kd-tree\n";
                else std::cout << "brute\n";
            }
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
                << "# 每个命令文件包含的最多命令数\n"
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
                << "diagramHeight=10\n";
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
    std::string dtp_flg;
    global_cfg.get_helper("useDetailedPrint", dtp_flg);
    global_detail_print = (dtp_flg == "true") ? true : false;
    std::string urd_flg;
    global_cfg.get_helper("useRangedDiagram", urd_flg);
    global_range_diag = (urd_flg == "true") ? true : false;
    global_cfg.get_helper("diagramHeight", global_diag_height);


    std::cout << "Launch with config:\n"
                "allowStartCommand: " << global_allow_start << '\n'
            << "maxLinePerCommand: " << global_max_line << '\n'
            << "multiThreadCount: " << global_thread_cnt << '\n'
            << "startCommandFile: " << global_start_path << '\n'
            << "useRangedDiagram" << (global_range_diag ? "true" : "false") << '\n'
            << "useDetailedPrint: " << (global_detail_print ? "true" : "false") << '\n'
            << "diagramHeight: " << global_diag_height << "\n\n";
    
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
        std::cout << "No console args found, start cmd mode.\n";   
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
        std::cout << "Command file execution finished, start cmd mode\n";
    }

    // Start command mode
    std::cout << '\n';
    std::string cmd_buffer;
    while (true) {
        std::cout << ">>> ";
        std::getline(std::cin, cmd_buffer);
        bool ret = executeCommand(cmd_buffer);
        if (ret) std::cout << "-> Success!\n\n";
        else std::cout << "-> Failed...\n\n";
    }
    return 0;
}