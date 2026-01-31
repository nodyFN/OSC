#include <stdint.h>
#include "shell.h"
#include "uart.h"
#include "string.h"
#include "sbi.h"

void getCommand(char* buffer, int max_len) {
    int cursor_idx = 0;
    int length = 0;
    
    // 初始化 buffer
    for(int i=0; i<max_len; i++) buffer[i] = '\0';

    while(1) {
        char c = uart_getc();

        // --- 1. ESC 處理 (方向鍵) --- 
        if (c == KEY_ESC) {
            // ... (維持你原本的方向鍵程式碼，不需要動) ...
            char next1 = uart_getc();
            if (next1 == '[') {
                char next2 = uart_getc();
                if (next2 == 'D') { // Left
                    if (cursor_idx > 0) {
                        cursor_idx--;
                        uart_puts("\033[D");
                    }
                } else if (next2 == 'C') { // Right
                    if (cursor_idx < length) {
                        cursor_idx++;
                        uart_puts("\033[C");
                    }
                }
            }
            continue;
        }

        // --- 2. Backspace 處理 (重點修改) ---
        else if (c == KEY_BACKSPACE || c == '\b' || c == 127) {
            if (cursor_idx > 0) {
                // A. 記憶體操作：將後面的字元往前搬
                // 假設 buffer: [A, B, C, D, E], cursor 在 C (index 2) 後面，要刪 B (index 1)
                // 我們要把 C, D, E 往前移
                
                int delete_idx = cursor_idx - 1; // 要被刪除的那個字的 index
                
                // 開始搬移 (Shift Left)
                for (int i = delete_idx; i < length - 1; i++) {
                    buffer[i] = buffer[i + 1];
                }
                
                // 更新長度與結尾
                length--;
                buffer[length] = '\0';
                cursor_idx--; // 游標也要倒退一格

                // B. 畫面操作：重新繪製後半段
                
                uart_putc('\b'); // 1. 物理游標先倒退一格 (回到被刪除的位置)

                // 2. 把後面剩下的字串印出來 (覆蓋舊的內容)
                for (int i = cursor_idx; i < length; i++) {
                    uart_putc(buffer[i]);
                }

                // 3. 印一個空白，蓋掉原本字串最尾端的殘影
                uart_putc(' ');

                // 4. 把游標移回正確的輸入位置
                // 我們剛剛印了 (length - cursor_idx) 個字 + 1 個空白
                // 所以要倒退這麼多次
                for (int i = 0; i < (length - cursor_idx) + 1; i++) {
                    uart_putc('\b');
                }
            }
        } 
        
        // --- 3. Enter ---
        else if (c == KEY_ENTER) {
            uart_puts("\r\n");
            buffer[length] = '\0';
            break;
        } 
        
        // --- 4. 一般輸入 (這裡還是覆寫模式 Overwrite Mode) ---
        // 如果你也想要 "插入模式 (Insert Mode)"，這裡也需要像上面一樣做 Shift Right
        else {
            // 檢查是否還有空間 (注意要預留 \0 的位置，且插入會增加總長度)
            if (length < max_len - 1) {
                
                // A. 記憶體操作：往右搬移 (Shift Right)
                // 必須從「最後面」開始搬，不然會覆蓋掉前面的資料
                // 假設 Buffer: [A, B, C], cursor 在 A(0) 後面
                // 我們要讓 B, C 變成 index 2, 3
                for (int i = length; i > cursor_idx; i--) {
                    buffer[i] = buffer[i - 1];
                }
                
                // 插入新字元
                buffer[cursor_idx] = c;
                
                // 更新長度
                length++;
                buffer[length] = '\0'; // 補上結尾

                // B. 畫面操作：重新繪製
                
                // 1. 印出新打的這個字
                uart_putc(c); 
                
                // 2. 把剛剛被往右擠的字串後半段全部印出來
                for (int i = cursor_idx + 1; i < length; i++) {
                    uart_putc(buffer[i]);
                }

                // 3. 把游標移回正確的位置
                // 因為我們剛剛印了 "新字元" + "後半段字串"，游標現在跑到最後面去了
                // 我們要把游標移回 cursor_idx + 1 的位置 (即新字元的後面)
                // 需要倒退的次數 = (目前總長度) - (新游標位置)
                // 新游標位置是 cursor_idx + 1
                int distance_to_back = length - (cursor_idx + 1);
                
                for (int i = 0; i < distance_to_back; i++) {
                    // 使用 VT100 左移指令，或者用 '\b' 也可以
                     uart_puts("\033[D"); 
                }

                // C. 更新邏輯游標
                cursor_idx++;
            }
        }
    }
}

void processCommand(shell_t* shell) {

    uart_puts("-> ");
    uart_puts(shell->command);
    uart_puts("\n");

    if(strcmp(shell->command, "reboot") == 0) {
        uart_puts("Rebooting...\n");
        sbi_legacy_reboot();
        uart_puts("System Halt (Reboot failed, please press Reset button).\n");
        // uart_puts("get reboot\n");
    } else if(strcmp(shell->command, "shutdown") == 0) {
        // uart_puts("Shutting down...\n");
        // uart_puts("get shutdown\n");
    }else{

    }


}

void runAShell(int32_t pid) {
    char* prompt_start = "OSClab> ";
    shell_t shell; 
    char command_buffer[128]; 
    shell.pid = pid;
    uart_puts(prompt_start);

    // 3. 把 buffer 傳進去填寫
    getCommand(command_buffer, 128);
    
    // 4. 指派給 shell struct
    shell.command = command_buffer; 

    // 5. 呼叫 processCommand (假設它接收指標，所以用 &shell)
    processCommand(&shell); 
}