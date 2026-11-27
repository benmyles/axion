#include "axion/tutor.hpp"
#include "axion/journal.hpp"
#include <charconv>
#include <iostream>
#include <iomanip>
#include <limits>
#include <sstream>
#include <stdexcept>

namespace axion {
namespace {

Word receptors(const std::string &s) {
    if (s == "_") return 0;
    if (s.starts_with("0x")) {
        Word bits{};
        auto [end, ec] = std::from_chars(s.data() + 2, s.data() + s.size(), bits, 16);
        if (ec != std::errc{} || end != s.data() + s.size()) throw std::invalid_argument("invalid receptor word " + s);
        return bits;
    }
    Word hash = 2166136261u;
    for (unsigned char ch : s) hash = (hash ^ ch) * 16777619u;
    return symbol(0x31a78653u, mix(hash));
}

void end_of_line(std::istringstream &line) {
    std::string extra;
    if (line >> extra) throw std::invalid_argument("unexpected field " + extra);
}

} // namespace

void tutor(Engine &engine, Model &model, std::istream &input, std::ostream &output, bool interactive) {
    if (model.animals != 1) throw std::invalid_argument("tutor requires a single-animal checkpoint");
    output << "ask A B C LEFT RIGHT, then reward + or reward -. Use pick for a frozen probe.\n"
           << "demo IMAGE TOOL RESULT teaches a visual effect. draw IMAGE TOOL [TOOL ...] predicts pixels. make TARGET TOOL [TOOL ...] constructs an input.\n";
    Course trial = model.course;
    trial.task = Task::signals;
    trial.scrambled = 0;
    Word trial_index = model.cursor;
    bool pending = false;
    unsigned chosen = 0;
    std::string candidates[2];
    std::string text;
    for (unsigned lineno = 1; std::getline(input, text); ++lineno) {
        std::istringstream line(text);
        std::string op;
        if (!(line >> op) || op.starts_with("#")) continue;
        try {
            if (op == "end") { end_of_line(line); break; }
            if (op == "demo" || op == "draw" || op == "make") {
                if (pending) throw std::invalid_argument("finish the pending ask with reward");
                std::string canvas, word;
                if (!(line >> canvas >> word)) throw std::invalid_argument("expected an image and a tool cue");
                Course panel = trial;
                panel.task = Task::canvas;
                panel.novel = op != "demo";
                panel.inverse = op == "make";
                panel.chain = 1;
                panel.signals.objects[0] = receptors(canvas);
                panel.signals.objects[1] = receptors(word);
                if (op == "demo") {
                    if (!(line >> word)) throw std::invalid_argument("demo needs an observed result image");
                    panel.signals.objects[4] = receptors(word);
                    end_of_line(line);
                    if (model.cursor == std::numeric_limits<Word>::max())
                        throw std::overflow_error("training cursor is full");
                } else {
                    while (line >> word) {
                        if (panel.chain == 3) throw std::invalid_argument("draw accepts up to three tool cues");
                        panel.signals.objects[1 + panel.chain++] = receptors(word);
                    }
                }
                auto result = engine.run(panel, model.cursor, 1, op != "demo" ? Learning::frozen : model.learning);
                if (auto *j=journal::current()) ++j->metrics[op=="demo" ? "demonstrations" : "drawing probes"];
                if (op == "demo") ++model.cursor;
                output << (panel.inverse ? "input 0x" : "prediction 0x") << std::hex << std::setw(8) << std::setfill('0')
                       << result.animals[0].image << std::dec << std::setfill(' ') << '\n' << std::flush;
                continue;
            }
            if (op == "reward") {
                std::string val;
                if (!pending) throw std::invalid_argument("reward requires a preceding ask");
                if (!(line >> val) || (val != "+" && val != "-" && val != "left" && val != "right"))
                    throw std::invalid_argument("reward expects +, -, left or right");
                end_of_line(line);
                if (model.cursor == std::numeric_limits<Word>::max())
                    throw std::overflow_error("training cursor is full");
                unsigned preferred = val == "+" ? chosen : (val == "-" ? 1 - chosen : unsigned(val == "right"));
                trial.signals.outcome = preferred;
                trial.signals.action = int(chosen);
                // Reconstruct the same spike train and committed action to apply
                // delayed feedback. The checkpoint remains at trial boundaries.
                engine.run(trial, trial_index, 1, model.learning);
                if (auto *j=journal::current()) { ++j->metrics["feedback"];j->metrics["rewarded"]+=preferred==chosen; }
                ++model.cursor;
                pending = false;
                output << "recorded " << (preferred == chosen ? "+1" : "-1") << '\n' << std::flush;
                continue;
            }
            if (op != "pick" && op != "ask") throw std::invalid_argument("expected ask, reward, pick or end");
            if (pending) throw std::invalid_argument("finish the pending ask with reward");
            for (int j = 0; j < Scene::slots; ++j) {
                std::string name;
                if (!(line >> name)) throw std::invalid_argument("a scene needs three cues and two alternatives");
                trial.signals.objects[j] = receptors(name);
                if (j >= 3) candidates[j - 3] = name;
            }
            end_of_line(line);
            trial.signals.outcome = 1;
            trial.signals.action = -1;
            trial_index = model.cursor;
            auto result = engine.run(trial, trial_index, 1, Learning::frozen);
            chosen = unsigned(result.animals.front().wins);
            if (auto *j=journal::current()) ++j->metrics["choices"];
            output << "choice " << (chosen ? "right " : "left ") << candidates[chosen] << '\n' << std::flush;
            pending = op == "ask";
        } catch (const std::invalid_argument &err) {
            std::string message = "tutor line " + std::to_string(lineno) + " " + err.what();
            if (!interactive) throw std::invalid_argument(message);
            output << message << '\n' << std::flush;
        }
    }
    if (pending) output << "Ended before feedback on the last choice.\n";
}
} // namespace axion
