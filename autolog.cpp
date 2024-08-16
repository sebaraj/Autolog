#include <algorithm>
#include <cctype>
#include <chrono>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

// Function to get the current date and time in the desired format
std::string getCurrentTime() {
  auto now = std::chrono::system_clock::now();
  auto now_c = std::chrono::system_clock::to_time_t(now);
  std::ostringstream oss;
  oss << std::put_time(std::localtime(&now_c), "%m/%d %I:%M%p");
  return oss.str();
}

// Function to calculate the time difference
std::string calculateTimeDifference(const std::string &startTimeStr) {
  // Define the format for the input time
  std::tm startTime = {};
  std::istringstream ss(startTimeStr);
  ss >> std::get_time(&startTime, "%m/%d  %I:%M%p");

  if (ss.fail()) {
    throw std::runtime_error("Failed to parse time string");
  }
  // Get the current time
  std::time_t now = std::time(nullptr);
  std::tm *nowTm = std::localtime(&now);

  // Calculate the difference in seconds
  std::time_t startEpoch = std::mktime(&startTime);
  std::time_t nowEpoch = std::mktime(nowTm);
  int difference = static_cast<int>(std::difftime(nowEpoch, startEpoch));

  // Convert the difference to hours and minutes
  int hours = difference / 3600;
  int minutes = (difference % 3600) / 60;

  // Format the result as "HH:MM"
  std::ostringstream result;
  result << std::setfill('0') << std::setw(2) << hours << ":"
         << std::setfill('0') << std::setw(2) << minutes;

  return result.str();
}

// Function to trim leading and trailing spaces
std::string trim(const std::string &str) {
  std::string trimmed = str;
  trimmed.erase(trimmed.begin(), std::find_if(trimmed.begin(), trimmed.end(),
                                              [](unsigned char ch) {
                                                return !std::isspace(ch);
                                              }));
  trimmed.erase(std::find_if(trimmed.rbegin(), trimmed.rend(),
                             [](unsigned char ch) { return !std::isspace(ch); })
                    .base(),
                trimmed.end());
  return trimmed;
}

// Function to search for the log file
std::string findLogFile() {
  fs::path currentPath = fs::current_path();
  while (currentPath.has_parent_path()) {
    for (const auto &entry : fs::directory_iterator(currentPath)) {
      if (entry.is_regular_file()) {
        std::ifstream file(entry.path());
        std::string firstLine;
        std::getline(file, firstLine);
        if (firstLine.find("ESTIMATE of time to complete assignment") !=
            std::string::npos) {
          return entry.path().string();
        }
      }
    }
    currentPath = currentPath.parent_path();
  }
  return "";
}

// Function to update the total time spent in the log file
void updateTotalTime(std::ifstream &file, std::string &logContent,
                     const std::string &newTimeSpent) {
  std::string line;
  std::string totalTimeSpent = "00:00";
  while (std::getline(file, line)) {
    logContent += line + "\n";
    if (line.find("TOTAL time spent") != std::string::npos) {
      size_t pos = line.find("TOTAL time spent");
      std::string timeSpent = line.substr(pos - 6, 5);
      int hours = std::stoi(timeSpent.substr(0, 2)) +
                  std::stoi(newTimeSpent.substr(0, 1));
      int minutes = std::stoi(timeSpent.substr(3, 2)) +
                    std::stoi(newTimeSpent.substr(2, 2));
      hours += minutes / 60;
      minutes %= 60;
      std::ostringstream oss;
      oss << std::setw(2) << std::setfill('0') << hours << ":" << std::setw(2)
          << std::setfill('0') << minutes;
      totalTimeSpent = oss.str();
    }
  }
  logContent = logContent.substr(0, logContent.find_last_of('\n')) +
               totalTimeSpent + "\n";
}

// Function to create the log file
void createLogFile(const std::string &fileName) {
  std::ofstream file(fileName);
  file << "ESTIMATE of time to complete assignment: \n\n"
       << "       Time     Time\n"
       << "Date   Started  Spent  Work Completed\n"
       << "-----  -------  -----  --------------\n\n"
       << "                -----\n"
       << "                00:00  TOTAL time spent\n\n"
       << "I discussed my solution with: \n\n"
       << "DISCUSSION:\n\n\n";
  file.close();
}

// Function to start logging
void startLog() {
  std::string logFile = findLogFile();
  if (logFile.empty()) {
    std::cerr << "Error: No log file found.\n";
    return;
  }

  std::string currentTime = getCurrentTime();

  std::ifstream file(logFile);
  std::vector<std::string> logLines;
  std::string line;
  bool foundTotalTimeSpent = false;

  // Read all lines except the "TOTAL time spent" line
  while (std::getline(file, line)) {
    if (line.find("                -----") != std::string::npos) {
      foundTotalTimeSpent = true;
      break;
    }
    logLines.push_back(line);
  }
  // file.close();

  if (!foundTotalTimeSpent) {
    std::cerr << "Error: 'TOTAL time spent' line not found.\n";
    return;
  }
  logLines.pop_back();
  //  Check if the last entry is incomplete by looking at the last non-empty
  //  line
  if (!logLines.empty()) {
    std::string lastLine = logLines.back();
    if (lastLine.find("  ") != std::string::npos && lastLine.length() <= 20) {
      // An incomplete entry: time added but no work description
      std::cerr << "Error: Previous entry not completed with stop command.\n";
      return;
    }
  }

  // Insert the new start time entry before the "TOTAL time spent" line
  logLines.push_back(currentTime.substr(0, 5) + " " + currentTime.substr(5) +
                     "   ");

  // Write back all lines including the "TOTAL time spent" line
  std::ofstream outFile(logFile);
  for (const auto &logLine : logLines) {
    outFile << logLine << "\n";
  }
  outFile << "\n                -----\n";
  outFile << file.rdbuf();
  file.close();
  outFile.close();
}

// Function to stop logging
void stopLog() {
  std::string logFile = findLogFile();
  if (logFile.empty()) {
    std::cerr << "Error: No log file found.\n";
    return;
  }

  std::ifstream file(logFile);
  std::vector<std::string> logLines;
  std::string line;
  std::string startTime;
  std::string timeSpent;
  int totalMinutesSpent = 0;
  bool foundIncompleteEntry = false;

  for (int i = 0; i < 5; i++) {
    std::getline(file, line);
    logLines.push_back(line);
  }
  // Read all lines and calculate total time spent
  while (std::getline(file, line)) {
    if (line.find("                -----") != std::string::npos) {
      break;
    }

    // Check for the incomplete entry
    if (line.find("   ") != std::string::npos && !foundIncompleteEntry) {
      startTime = line.substr(0, 14); // Extract start time
      timeSpent = calculateTimeDifference(startTime);
      line = line.substr(0, 16) + timeSpent +
             "  "; // Update the line with the time spent
      foundIncompleteEntry = true;
    }

    if (line.length() >= 20 &&
        line.substr(14, 5).find(":") != std::string::npos) {
      // Extract time spent and add it to the total
      std::string time = line.substr(14, 5);
      std::cout << time << std::endl; // remove
      int hours = std::stoi(time.substr(0, 2));
      int minutes = std::stoi(time.substr(3, 2));
      totalMinutesSpent += (hours * 60) + minutes;
    }

    logLines.push_back(line);
  }

  if (!foundIncompleteEntry) {
    std::cerr << "Error: No incomplete entry found.\n";
    return;
  }

  std::cout << "Enter work completed: ";
  std::string workCompleted;
  std::getline(std::cin, workCompleted);
  logLines.pop_back();
  logLines.back() += workCompleted; // Append work completed to the last entry

  // Calculate the total time spent after adding the new entry
  int newHours = std::stoi(timeSpent.substr(0, 2));
  int newMinutes = std::stoi(timeSpent.substr(3, 2));
  totalMinutesSpent += (newHours * 60) + newMinutes;
  int totalHours = totalMinutesSpent / 60;
  int totalMinutes = totalMinutesSpent % 60;

  std::ostringstream totalTimeStr;
  totalTimeStr << std::setw(2) << std::setfill('0') << totalHours << ":"
               << std::setw(2) << std::setfill('0') << totalMinutes;

  // Write back all lines including the "TOTAL time spent" line
  std::ofstream outFile(logFile);
  for (const auto &logLine : logLines) {
    outFile << logLine << "\n";
  }
  outFile << "\n                -----\n"
          << "                " << totalTimeStr.str() << "  TOTAL time spent\n";
  std::getline(file, line);
  outFile << file.rdbuf();
  file.close();
  outFile.close();
}

// Function to add collaborators
void addCollaborator(int argc, char *argv[]) {
  if (argc < 3) {
    std::cerr << "Error: No collaborator name provided.\n";
    return;
  }

  std::string logFile = findLogFile();
  if (logFile.empty()) {
    std::cerr << "Error: No log file found.\n";
    return;
  }

  // Combine all arguments after "collab" into one string
  std::ostringstream collaboratorStream;
  for (int i = 2; i < argc; ++i) {
    if (i > 2) {
      collaboratorStream << " ";
    }
    collaboratorStream << argv[i];
  }
  std::string collaborator = collaboratorStream.str();

  std::ifstream file(logFile);
  std::string logContent;
  std::string line;
  while (std::getline(file, line)) {
    if (line.find("I discussed my solution with:") != std::string::npos) {
      if (line.back() != ' ') {
        line += ", ";
      }
      line += collaborator;
    }
    logContent += line + "\n";
  }
  file.close();

  std::ofstream outFile(logFile);
  outFile << logContent;
  outFile.close();
}

// Function to add a collaborator
void addCollaborator(const std::string &firstName,
                     const std::string &lastName) {
  std::string logFile = findLogFile();
  if (logFile.empty()) {
    std::cerr << "Error: No log file found.\n";
    return;
  }

  std::ifstream file(logFile);
  std::string logContent;
  std::string line;
  while (std::getline(file, line)) {
    if (line.find("I discussed my solution with:") != std::string::npos) {
      if (line.back() != ' ') {
        line += ", ";
      }
      line += firstName + " " + lastName;
    }
    logContent += line + "\n";
  }
  file.close();

  std::ofstream outFile(logFile);
  outFile << logContent;
  outFile.close();
}

// Main function
int main(int argc, char *argv[]) {
  if (argc < 2) {
    std::cerr << "Usage: autolog <command> [arguments]\n";
    return 1;
  }

  std::string command = argv[1];

  if (command == "create") {
    if (argc < 3) {
      std::cerr << "Usage: autolog create <filename>\n";
      return 1;
    }
    createLogFile(argv[2]);

    std::cout << "Enter your estimate of time to complete the assignment: ";
    std::string estimate;
    std::getline(std::cin, estimate);
    estimate = trim(estimate); // Trim the input

    // Read the entire file content and insert the estimate
    std::ifstream file(argv[2]);
    std::string line;
    std::ostringstream oss;
    bool estimateInserted = false;

    while (std::getline(file, line)) {
      if (line.find("ESTIMATE of time to complete assignment:") !=
              std::string::npos &&
          !estimateInserted) {
        oss << line << "" << estimate << " hours" << "\n";
        estimateInserted = true;
      } else {
        oss << line << "\n";
      }
    }
    file.close();

    // Write the updated content back to the file
    std::ofstream fileWrite(argv[2]);
    fileWrite << oss.str();
    fileWrite.close();
  } else if (command == "start") {
    startLog();
  } else if (command == "stop") {
    stopLog();
  } else if (command == "collab") {
    addCollaborator(argc, argv);
  } else {
    std::cerr << "Unknown command: " << command << "\n";
    return 1;
  }

  return 0;
}
