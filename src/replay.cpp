#include "axion/replay.hpp"
#include <iomanip>
#include <ostream>
#include <stdexcept>

namespace axion {
namespace {
void raster(std::ostream &out, Word a, Word b, Word c) {
    for (int y = 0; y < 4; ++y) {
        for (Word image : {a, b, c}) {
            for (int x = 0; x < 8; ++x) out << ((image & (1u << (31 - 8*y - x))) ? "##" : "..");
            out << "    ";
        }
        out << '\n';
    }
}

void hex_word(std::ostream &out, Word word) {
    out << "0x" << std::hex << std::setw(8) << std::setfill('0') << word << std::dec << std::setfill(' ');
}
}

void replay(std::ostream &out, Course c, Word animal, Word trial, const Totals &observed) {
    Word who = c.common_trials ? 0 : animal;
    Word key = trial_key(c.seed, who, trial);
    out << "Animal " << animal << ", held-out trial " << trial << '\n';
    if (c.task == Task::routes || c.task == Task::errand) {
        Maze maze = Maze::make(c.lesson, who);
        Word room = key & 15;
        auto goals = Destination::draw(maze, room, key, true, c.task == Task::errand);
        out << "Goal rooms  " << char('A' + goals.first);
        if (goals.count == 2) out << " then " << char('A' + goals.second);
        out << "\nCued receptor patterns ";
        for (int j = 0; j < goals.count; ++j) {
            Word place = (j ^ int(c.task == Task::errand && c.swap_words)) ? goals.second : goals.first;
            Word id = c.task == Task::errand ? name_id(maze.wiring, place) : place;
            out << ' ';
            hex_word(out, c.mute_odor ? 0 : symbol(maze.vocabulary, id));
        }
        out << "\nObserved path  " << char('A' + room);
        int length = int(observed.ticks / 3) - goals.count - 1, reached = 0;
        if (length < 0 || length > 6 * goals.count) throw std::runtime_error("invalid route trace length");
        for (int j = 0; j < length; ++j) {
            int door = (observed.image >> j) & 1;
            room = maze.step(room, door ^ int(c.reversed));
            out << (door ? " --R--> " : " --L--> ") << char('A' + room);
            if (reached < goals.count && room == (reached ? goals.second : goals.first))
                out << '[' << ++reached << ']';
        }
        if ((reached == goals.count) != bool(observed.wins)) throw std::runtime_error("route trace and result disagree");
        out << "\nReached " << reached << " of " << goals.count << " destinations in " << length << " moves.\n";
    } else if (is_drawing(c.task)) {
        Workshop world = Workshop::make(c, who);
        Word original = c.mute_odor ? 0 : Workshop::picture(c, who, trial);
        Word ids = Workshop::sequence(c, key, trial, c.chain);
        Word target = original, actual = observed.image;
        out << "Tool sequence ";
        for (Word j = 0; j < c.chain; ++j) {
            int tool = (ids >> j) & 1;
            out << ' ' << char('A' + tool);
            target = world.apply(target, tool, c.difficult);
            if (c.task == Task::invention) actual = world.apply(actual, tool, c.difficult);
        }
        if (Word(32 - population(actual ^ target)) != observed.matches)
            throw std::runtime_error("rendered picture and returned score disagree");
        if (c.task == Task::invention) {
            out << "\nDesired             Chosen input        Actual result\n";
            raster(out, target, observed.image, actual);
        } else {
            out << "\nInput               Predicted           Actual result\n";
            raster(out, original, observed.image, target);
        }
        out << observed.matches << " of 32 pixels match.\n";
    } else throw std::invalid_argument("replay supports routes, errand, workshop and invention");
}
} // namespace axion
