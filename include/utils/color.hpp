#pragma once

#include <string>

namespace mud::utils::color {

const std::string RESET = "\033[0m";

// Message types
const std::string SAY     = "\x1b[32m";    // Green          (일반 채팅 - 편안함)
const std::string SHOUT   = "\x1b[1;33m";  // Bright Yellow  (외치기 - 강한 주목)
const std::string WHISPER = "\x1b[1;35m";    // Magenta        (귓속말 - 은밀함)

const std::string SYSTEM  = "\x1b[1;34m";  // Bright Blue    (시스템 - 정보성)
const std::string MOVE    = "\x1b[1;36m";  // Bright Cyan    (이동 - 흐름/행동)

// const std::string JOIN    = "\x1b[1;36m";  // Bright Cyan    (입장 - 밝고 긍정적)
const std::string JOIN    = "\x1b[32m";    // Cyan           (입장 - 밝고 긍정적)
const std::string LEFT    = "\x1b[90m";    // Bright Gray    (퇴장 - 조용하고 절제됨)

const std::string ERROR_  = "\x1b[1;91m";  // Bright Red     (에러 - 강한 경고)
// const std::string EVENT   = "\x1b[1;35m";  // Bright Magenta (이벤트 - 특별함/이끌림)
const std::string EVENT = "\x1b[1;33m";      // Bright Yellow  (이벤트 - 특별함/이끌림)
const std::string PORTAL  = "\x1b[1;35m";  // Bright Magenta (포탈 - 신비로움/탐험)


inline std::string tag(const std::string &tag, const std::string &color,
                       const std::string &message) {
  return color + "[" + tag + "] " + RESET + message;
}

inline std::string color(const std::string &color, const std::string &message) {
  return color + message + RESET;
}

inline std::string tagWithColor(const std::string &tag, const std::string &color,
                            const std::string &message) {
  return color + "[" + tag + "] " + message + RESET;
}

inline std::string say(const std::string &message) {
  return tag("Say", SAY, message);
}

inline std::string shout(const std::string &message) {
  return tagWithColor("Shout", SHOUT, message);
}

inline std::string whisper(const std::string &message) {
  return tagWithColor("Whisper", WHISPER, message);
}

inline std::string join(const std::string &message) {
  return tagWithColor("Join", JOIN, message);
}

inline std::string left(const std::string &message) {
  return tagWithColor("Left", LEFT, message);
}

inline std::string event(const std::string &message) {
  return tag("Event", EVENT, message);
}

inline std::string move(const std::string &message) {
  return tag("Move", MOVE, message);
}

inline std::string system(const std::string &message) {
  return tag("System", SYSTEM, message);
}

inline std::string error(const std::string &message) {
  return tag("Error", ERROR, message);
}

inline std::string portal(const std::string &message) {
  return tag("Portal", PORTAL, message);
}

} // namespace mud::utils::color
