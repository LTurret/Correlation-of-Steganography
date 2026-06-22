#include "menu.hpp"
#include "matrix_analysis.hpp"
#include "steganography.hpp"
#include <iostream>
#include <print>

namespace ui {

void display_welcome() {
    std::println("==================================================");
    std::println("      圖像序列化與不可視浮水印分析系統 (C++23)     ");
    std::println("==================================================");
}

void run_menu_loop() {
    while (true) {
        std::println("\n--- 功能選單 ---");
        std::println("1. 嵌入隱寫浮水印 (Embed)");
        std::println("2. 提取與驗證浮水印 (Extract & Verify)");
        std::println("3. 多載體關聯性矩陣分析 (Matrix Analysis)");
        std::println("4. 結束程式 (Exit)");
        std::print("請輸入選項 (1-4): ");

        std::string choice;
        std::getline(std::cin, choice);

        if (choice == "1") {
            std::string src, pwd, payload;
            std::print("輸入來源圖片路徑: ");
            std::getline(std::cin, src);
            std::print("輸入控制密碼: ");
            std::getline(std::cin, pwd);
            std::print("輸入情報明文內容: ");
            std::getline(std::cin, payload);

            // 移除原本傳入 user 的位置
            auto res = steg::embed_data(src, pwd, payload);
            if (res) {
                std::println("\n[嵌入成功]");
                std::println("輸出路徑: {}", res->output_path.string());
                std::println("原始圖片 MD5: {}", res->original_md5);
                std::println("修改圖片 MD5: {}", res->modified_md5);
            } else {
                std::println("\n[錯誤] 嵌入失敗。");
            }
        } else if (choice == "2") {
            std::string src, pwd;
            std::print("輸入含浮水印圖片路徑: ");
            std::getline(std::cin, src);
            std::print("輸入控制密碼: ");
            std::getline(std::cin, pwd);

            auto res = steg::extract_data(src, pwd);
            if (res) {
                std::println("\n[驗證成功] 標頭檢驗通過。");
                std::println("提取明文內容: {}", res->payload);
            } else {
                std::println("\n[驗證失敗] 密碼錯誤或未偵測到有效浮水印標頭。");
            }
        } else if (choice == "3") {
            std::string dir, pwd;
            std::print("輸入目標圖像資料夾路徑: ");
            std::getline(std::cin, dir);
            std::print("輸入控制密碼: ");
            std::getline(std::cin, pwd);

            auto res = analysis::analyze_directory(dir, pwd);
            if (res) {
                std::println("\n--- 多載體對稱關聯性矩陣 (C = A*A^T) ---");
                size_t m = res->file_names.size();

                std::print("{:<20}", "圖片名稱");
                for (size_t i = 0; i < m; ++i) {
                    std::print("{:>8}", i);
                }
                std::println("");

                for (size_t i = 0; i < m; ++i) {
                    std::print(
                        "{:<20}",
                        res->file_names[i].filename().string().substr(0, 18));
                    for (size_t j = 0; j < m; ++j) {
                        std::print("{:>8}", res->correlation_matrix[i][j]);
                    }
                    std::println("");
                }
                std::println("\n[矩陣解讀說明] "
                             "對角線為圖片自身特徵強度；非對角線數值越高，代表"
                             "該兩張圖片在該密碼空間下埋藏的隱寫關聯性越強。");
            } else {
                std::println(
                    "\n[錯誤] "
                    "分析失敗，請檢查路徑或資料夾內是否有相容格式圖片。");
            }
        } else if (choice == "4") {
            std::println("系統安全退出。");
            break;
        } else {
            std::println("無效的選項，請重新輸入。");
        }
    }
}

} // namespace ui