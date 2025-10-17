#include <Arduino.h>
#include <LittleFS.h>

#define LED_PIN 2        // основной светодиод (используется для индикации работы системы)
#define LED_PANIC 2     // светодиод D2 для kernel panic (можно разделить пины, если нужно)

bool kernelPanic = false;
String currentDir = "/"; // Текущая директория для поддержки навигации (всегда с trailing /)

// -------------------------------------------------
// ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ
// -------------------------------------------------
void printDivider() {
  Serial.println("----------------------------------------");
}

String readCommand() {
  while (!Serial.available()) {
    delay(10);
  }
  String cmd = Serial.readStringUntil('\n');
  cmd.trim(); // Удаляем лишние пробелы и \r если есть
  return cmd;
}

void triggerKernelPanic(const String& reason) {
  Serial.println("\n*** KERNEL PANIC ***");
  Serial.println("Reason: " + reason);
  kernelPanic = true;
  while (true) {
    digitalWrite(LED_PANIC, HIGH);
    delay(250);
    digitalWrite(LED_PANIC, LOW);
    delay(250);
  }
}

void blinkLed(int pin, int times, int delayMs) {
  for (int i = 0; i < times; i++) {
    digitalWrite(pin, HIGH);
    delay(delayMs);
    digitalWrite(pin, LOW);
    delay(delayMs);
  }
}

// Парсер команд с поддержкой кавычек для аргументов с пробелами
// Не поддерживает escaped кавычки, простая реализация
int parseArgs(const String& line, String args[], int maxArgs) {
  int idx = 0;
  bool inQuote = false;
  String current = "";
  for (int i = 0; i < line.length(); i++) {
    char c = line[i];
    if (c == '\"') {
      inQuote = !inQuote;
    } else if (c == ' ' && !inQuote) {
      if (current.length() > 0) {
        if (idx < maxArgs) args[idx++] = current;
        current = "";
      }
    } else {
      current += c;
    }
  }
  if (current.length() > 0 && idx < maxArgs) args[idx++] = current;
  return idx;
}

// Функция для разрешения полного пути с обработкой ., .. и множественных слешей
String resolvePath(const String& inputPath) {
  String path = inputPath;
  if (!path.startsWith("/")) {
    path = currentDir + path;
  }

  // Удаляем множественные слеши
  while (path.indexOf("//") != -1) {
    path.replace("//", "/");
  }

  // Строим результирующий путь, обрабатывая . и ..
  String result = "/";
  int pos = 1; // Начинаем после первого /
  while (pos < path.length()) {
    int nextSlash = path.indexOf('/', pos);
    if (nextSlash == -1) nextSlash = path.length();
    String component = path.substring(pos, nextSlash);
    pos = nextSlash + 1;

    if (component == "" || component == ".") {
      continue;
    }
    if (component == "..") {
      // Удаляем последний компонент из result
      if (result.length() > 1) {
        int lastSlash = result.lastIndexOf('/', result.length() - 2);
        result = result.substring(0, lastSlash + 1);
      }
    } else {
      result += component + "/";
    }
  }

  // Если не корень, удаляем trailing slash для consistency
  if (result.length() > 1 && result.endsWith("/")) {
    result = result.substring(0, result.length() - 1);
  }

  return result;
}

// Проверка, является ли путь директорией (удаляем trailing / перед открытием для безопасности)
bool isDirectory(const String& inputPath) {
  String path = inputPath;
  if (path.length() > 1 && path.endsWith("/")) {
    path = path.substring(0, path.length() - 1);
  }
  File f = LittleFS.open(path);
  if (f) {
    bool isDir = f.isDirectory();
    f.close();
    return isDir;
  }
  return false;
}

// Проверка на kernel panic (критические файлы)
void checkCriticalFiles() {
  if (!LittleFS.exists("/bootmode.txt") || !LittleFS.exists("/sys.kernel")) {
    triggerKernelPanic("Critical system file removed!");
  }
}

// -------------------------------------------------
// INIT
// -------------------------------------------------
void setup() {
  pinMode(LED_PIN, OUTPUT);
  pinMode(LED_PANIC, OUTPUT);

  // Индикация запуска
  blinkLed(LED_PIN, 3, 200);

  Serial.begin(115200);
  delay(200);

  if (!LittleFS.begin()) { // Без форматирования по умолчанию, чтобы избежать потери данных
    Serial.println("FS mount failed! Attempting format...");
    if (!LittleFS.format() || !LittleFS.begin()) {
      triggerKernelPanic("Filesystem initialization failed!");
    }
  }

  // Создаем критические файлы, если их нет
  if (!LittleFS.exists("/sys.kernel")) {
    File kf = LittleFS.open("/sys.kernel", "w");
    if (kf) {
      kf.print("FleshOS Kernel v2.1");
      kf.close();
    } else {
      triggerKernelPanic("Failed to create /sys.kernel!");
    }
  }

  if (!LittleFS.exists("/bootmode.txt")) {
    File bf = LittleFS.open("/bootmode.txt", "w");
    if (bf) {
      bf.print("fleshos");
      bf.close();
    } else {
      triggerKernelPanic("Failed to create /bootmode.txt!");
    }
  }

  Serial.println("=== FleshOS Boot ===");
  printDivider();
  Serial.println("Type 'help' to see available commands");
  printDivider();
  Serial.print("FleshOS " + currentDir + "> ");
}

// -------------------------------------------------
// FLESHOS COMMANDS
// -------------------------------------------------
void fleshosLoop() {
  if (!Serial.available()) return;
  String cmd = readCommand();
  if (cmd.length() == 0) { 
    Serial.print("FleshOS " + currentDir + "> "); 
    return; 
  }

  String args[10]; // Максимум 10 аргументов
  int argCount = parseArgs(cmd, args, 10);

  if (argCount > 0) {
    String command = args[0];

    // ---------------- commands ----------------
    if (command == "ls") {
      String dirPathArg = (argCount > 1) ? args[1] : "";
      String dirPath = (dirPathArg == "") ? currentDir : resolvePath(dirPathArg);
      // Удаляем trailing / для open
      if (dirPath.length() > 1 && dirPath.endsWith("/")) {
        dirPath = dirPath.substring(0, dirPath.length() - 1);
      }
      File root = LittleFS.open(dirPath);
      if (!root || !root.isDirectory()) {
        Serial.println("Not a directory or does not exist");
        if (root) root.close();
      } else {
        File file = root.openNextFile();
        while (file) {
          Serial.print(file.name());
          if (file.isDirectory()) {
            Serial.print(" (dir)");
          } else {
            Serial.print(" (" + String(file.size()) + " bytes)");
          }
          Serial.println();
          file.close();
          file = root.openNextFile();
        }
        root.close();
      }
    } 
    else if (command == "cat") {
      if (argCount < 2) {
        Serial.println("Usage: cat <file>");
      } else {
        String fname = resolvePath(args[1]);
        if (!LittleFS.exists(fname) || isDirectory(fname)) {
          Serial.println("File not found or is directory");
        } else if (fname == "/bootmode.txt") {
          // Пасхалка
          Serial.println("FleshOS/////////////////////Powered By Itry");
          Serial.println("version fleshos: 2.1");
          Serial.printf("modelESP: %s\n", ESP.getChipModel());
        } else {
          File f = LittleFS.open(fname, "r");
          if (!f) {
            Serial.println("Failed to open file");
          } else {
            while (f.available()) {
              Serial.write(f.read());
            }
            Serial.println();
            f.close();
          }
        }
      }
    } 
    else if (command == "rm") {
      if (argCount < 2) {
        Serial.println("Usage: rm <file>");
      } else {
        String fname = resolvePath(args[1]);
        if (fname == "/") {
          triggerKernelPanic("Attempt to remove root directory!");
        }
        if (!LittleFS.exists(fname)) {
          Serial.println("File not found");
        } else if (isDirectory(fname)) {
          Serial.println("Cannot remove directory with rm (use rmdir for empty dirs)");
        } else if (LittleFS.remove(fname)) {
          Serial.println("Deleted");
          checkCriticalFiles();
        } else {
          Serial.println("Cannot delete");
        }
      }
    } 
    else if (command == "rmdir") {
      if (argCount < 2) {
        Serial.println("Usage: rmdir <dir>");
      } else {
        String dname = resolvePath(args[1]);
        if (dname == "/") {
          triggerKernelPanic("Attempt to remove root directory!");
        }
        if (!LittleFS.exists(dname) || !isDirectory(dname)) {
          Serial.println("Directory not found");
        } else {
          // Проверяем, пустая ли директория
          File root = LittleFS.open(dname);
          File file = root.openNextFile();
          if (file) {
            Serial.println("Directory not empty");
            file.close();
            root.close();
          } else {
            root.close();
            if (LittleFS.rmdir(dname)) {
              Serial.println("Directory removed");
            } else {
              Serial.println("Failed to remove directory");
            }
          }
        }
      }
    }
    else if (command == "mkdir") {
      if (argCount < 2) {
        Serial.println("Usage: mkdir <dir>");
      } else {
        // Простая версия без -p; для рекурсивного создания нужно реализовать вручную
        String dname = resolvePath(args[1]);
        if (LittleFS.exists(dname)) {
          Serial.println("Directory already exists");
        } else if (LittleFS.mkdir(dname)) {
          Serial.println("Directory created");
        } else {
          Serial.println("Failed to create directory (parent may not exist)");
        }
      }
    } 
    else if (command == "write") {
      if (argCount < 3) {
        Serial.println("Usage: write <file> <text>");
      } else {
        String fname = resolvePath(args[1]);
        if (isDirectory(fname)) {
          Serial.println("Cannot write to directory");
        } else {
          String text = "";
          for (int i = 2; i < argCount; i++) {
            if (i > 2) text += " ";
            text += args[i];
          }
          File f = LittleFS.open(fname, "w");
          if (f) {
            f.print(text);
            f.close();
            Serial.println("File written");
          } else {
            Serial.println("Failed to write file");
          }
        }
      }
    } 
    else if (command == "append") {
      if (argCount < 3) {
        Serial.println("Usage: append <file> <text>");
      } else {
        String fname = resolvePath(args[1]);
        if (!LittleFS.exists(fname) || isDirectory(fname)) {
          Serial.println("File not found or is directory");
        } else {
          String text = "";
          for (int i = 2; i < argCount; i++) {
            if (i > 2) text += " ";
            text += args[i];
          }
          File f = LittleFS.open(fname, "a");
          if (f) {
            f.print(text);
            f.close();
            Serial.println("Text appended");
          } else {
            Serial.println("Failed to append to file");
          }
        }
      }
    } 
    else if (command == "touch") {
      if (argCount < 2) {
        Serial.println("Usage: touch <file>");
      } else {
        String fname = resolvePath(args[1]);
        if (LittleFS.exists(fname)) {
          Serial.println("File already exists");
        } else {
          File f = LittleFS.open(fname, "w");
          if (f) {
            f.close();
            Serial.println("File created");
          } else {
            Serial.println("Failed to create file");
          }
        }
      }
    }
    else if (command == "mv") {
      if (argCount < 3) {
        Serial.println("Usage: mv <source> <destination>");
      } else {
        String src = resolvePath(args[1]);
        String dest = resolvePath(args[2]);
        if (!LittleFS.exists(src)) {
          Serial.println("Source not found");
        } else if (LittleFS.rename(src, dest)) {
          Serial.println("Moved/renamed");
          checkCriticalFiles();
        } else {
          Serial.println("Failed to move/rename");
        }
      }
    }
    else if (command == "cp") {
      if (argCount < 3) {
        Serial.println("Usage: cp <source> <destination>");
      } else {
        String src = resolvePath(args[1]);
        String dest = resolvePath(args[2]);
        if (!LittleFS.exists(src) || isDirectory(src)) {
          Serial.println("Source not found or is directory");
        } else {
          File srcFile = LittleFS.open(src, "r");
          File destFile = LittleFS.open(dest, "w");
          if (srcFile && destFile) {
            while (srcFile.available()) {
              destFile.write(srcFile.read());
            }
            srcFile.close();
            destFile.close();
            Serial.println("Copied");
          } else {
            Serial.println("Failed to copy");
            if (srcFile) srcFile.close();
            if (destFile) destFile.close();
          }
        }
      }
    }
    else if (command == "df") {
      Serial.printf("Total bytes: %u\n", LittleFS.totalBytes());
      Serial.printf("Used bytes: %u\n", LittleFS.usedBytes());
      Serial.printf("Free bytes: %u\n", LittleFS.totalBytes() - LittleFS.usedBytes());
    }
    else if (command == "ping") {
      if (argCount < 2) {
        Serial.println("Usage: ping <addr>");
      } else {
        Serial.printf("Ping %s: OK (fake response)\n", args[1].c_str());
      }
    }
    else if (command == "cd") {
      String dnameArg = (argCount > 1) ? args[1] : "";
      String newDir;
      if (dnameArg == "") {
        newDir = "/";
      } else {
        newDir = resolvePath(dnameArg);
      }
      if (!LittleFS.exists(newDir) || !isDirectory(newDir)) {
        Serial.println("Directory not found");
      } else {
        currentDir = newDir;
        if (currentDir != "/" && !currentDir.endsWith("/")) currentDir += "/";
      }
    }
    else if (command == "pwd") {
      Serial.println(currentDir);
    }
    else if (command == "reboot") {
      Serial.println("Rebooting...");
      delay(500);
      ESP.restart();
    } 
    else if (command == "help") {
      printDivider();
      Serial.println("ls [dir]           - list files in directory");
      Serial.println("cat <file>         - show file content");
      Serial.println("rm <file>          - delete file");
      Serial.println("rmdir <dir>        - remove empty directory");
      Serial.println("mkdir <dir>        - create directory");
      Serial.println("write <file> <txt> - create/write file (supports quotes for spaces)");
      Serial.println("append <file> <txt>- append text to file (supports quotes for spaces)");
      Serial.println("touch <file>       - create empty file");
      Serial.println("mv <src> <dest>    - move/rename file or dir");
      Serial.println("cp <src> <dest>    - copy file");
      Serial.println("df                 - show filesystem usage");
      Serial.println("ping <addr>        - ping address (fake)");
      Serial.println("cd [dir]           - change directory");
      Serial.println("pwd                - print working directory");
      Serial.println("reboot             - reboot ESP");
      printDivider();
    } 
    else {
      Serial.println("Unknown command. Type 'help' for list.");
    }
  }

  Serial.print("FleshOS " + currentDir + "> ");
}

// -------------------------------------------------
// MAIN LOOP
// -------------------------------------------------
void loop() {
  if (!kernelPanic) {
    fleshosLoop();
    // LED мигает медленнее для индикации активности
    digitalWrite(LED_PIN, HIGH);
    delay(100);
    digitalWrite(LED_PIN, LOW);
    delay(100);
  }
}
