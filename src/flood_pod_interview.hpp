class PodInterview {
    Session &lab;
    AgentImage &individual;
    DevicePtr eye;
    Temporary organ, pointed, reading;
    Function gesture{}, feed{};
    Word seed = 0, angle = 0;
    bool open = false, turned = false;
    Word encountered = 0;
    static Word quantity(const std::string &value, Word maximum) {
        Word n = 0;
        auto [end, err] = std::from_chars(value.data(), value.data() + value.size(), n);
        if (err != std::errc{} || end != value.data() + value.size() || n > maximum)
            throw std::invalid_argument("invalid feeding tray quantity");
        return n;
    }

  public:
    PodInterview(Session &s, AgentImage &subject, DevicePtr grounded)
        : lab(s), individual(subject), eye(grounded), organ(s.d, subject.delayed_pods ? sizeof(pods::Animal) : 0),
          pointed(s.d, subject.delayed_pods ? 2 * sizeof(Count) : 0),
          reading(s.d, subject.delayed_pods ? sizeof(pods::TrayReading) : 0) {}
    bool control(const std::string &line) {
        if (!organ.ptr || line.empty() || line[0] != '/')
            return false;
        std::istringstream words(line);
        std::string name;
        words >> name;
        if (name != "/tidalpool" && name != "/ripen" && name != "/offer" && name != "/turn" && name != "/grip" &&
            name != "/grip-word" && name != "/handle")
            return false;
        std::vector<std::string> args;
        for (std::string v; words >> v;)
            args.push_back(v);
        if (args.empty() || args.size() > (name == "/handle" || name == "/offer" ? 2u : 1u))
            throw std::invalid_argument("incorrect feeding tray arguments");
        if (name == "/tidalpool") {
            seed = quantity(args[0], ~0u);
            angle = 0;
            turned = false;
            encountered = 0;
            if (!open) {
                lab.bind(feed, "flood_pod_tray", Image::floodplain);
                lab.bind(gesture, "ground_point", Image::grounding);
                lab.d.check(lab.d.upload(organ.ptr, &individual.pod_organ, sizeof(pods::Animal)),
                            "seat this resident at the feeding tray");
                open = true;
            }
            std::cout << "tray_seed=" << seed << "  tick=" << individual.resident.tick << std::endl;
            return true;
        }
        if (!open) {
            std::cout << "Open the feeding tray with /tidalpool SEED." << std::endl;
            return true;
        }
        if (name == "/turn") {
            angle = (angle + quantity(args[0], 3)) & 3;
            turned = true;
            encountered = 0;
            std::cout << "The tray turned." << std::endl;
            return true;
        }
        bool verbal = name == "/grip-word" || name == "/handle";
        Word action = name != "/grip" && name != "/grip-word", variety = 8, duration = 1,
             start = individual.resident.tick;
        if (verbal) {
            if (!eye) {
                std::cout << "The resident has no visual word circuit." << std::endl;
                return true;
            }
            Count heard = acoustic::pulse(args[0]);
            void *point_args[]{&eye, &heard, &pointed.ptr};
            lab.d.check(lab.d.launch(gesture, 1, 1, 1, 1, 1, 1, 0, nullptr, point_args, nullptr),
                        "point to the heard form");
            variety = 9;
        } else if (name != "/ripen")
            variety = quantity(args[0], 7);
        if (args.size() == 2)
            duration = quantity(args[1], 8192);
        if (name == "/ripen")
            duration = quantity(args[0], 8192);
        if (!duration || (action && duration > ~0u - start))
            throw std::invalid_argument("invalid feeding interval");
        void *kernel_args[]{&organ.ptr, &eye,      &pointed.ptr, &seed,   &angle,
                            &start,     &duration, &variety,     &action, &reading.ptr};
        launch(lab, feed, 1, kernel_args);
        pods::TrayReading event{};
        lab.d.check(lab.d.download(&event, reading.ptr, sizeof(event)), "read this resident's muscle output");
        auto *journal_run = journal::current();
        if (verbal && journal_run)
            ++journal_run->metrics["tray_word_requests"];
        if (event.ambiguous) {
            if (journal_run)
                ++journal_run->metrics["tray_unresolved"];
            std::cout << "grip=unresolved  handling=" << action << std::endl;
            return true;
        }
        Word absorbed = 0;
        if (action) {
            auto &a = individual.resident.animal;
            individual.resident.tick += event.ticks;
            for (Word k = 0; k < 3; ++k) {
                Word retained = std::min(65535u - Word(a.energy), event.edible[k]);
                a.energy += retained;
                a.diet[k] += retained;
                a.meals += retained;
                absorbed += retained;
            }
            lab.d.check(lab.d.download(&individual.pod_organ, organ.ptr, sizeof(pods::Animal)),
                        "retain the resident's experience");
        }
        if (journal_run) {
            auto &q = journal_run->metrics;
            q["tray_ticks"] += event.ticks;
            q["tray_bites"] += event.bites;
            q["tray_opened"] += event.opened;
            q["tray_absorbed"] += absorbed;
            q["tray_located"] += verbal;
            if (verbal && !action && turned && encountered && !(encountered & event.surfaces)) {
                ++q["tray_untried"];
                q["tray_untried_correct"] += event.aligned;
            }
        }
        if (action)
            encountered |= event.surfaces;
        std::ostringstream row;
        row << "tray_grip=" << event.muscle << "  light=" << event.light << "  site=" << event.site
            << "  ticks=" << event.ticks << "  bites=" << event.bites << "  opened=" << event.opened
            << "  absorbed=" << absorbed << "  husks=" << event.husks << "  named=" << verbal;
        std::cout << row.str() << std::endl;
        return true;
    }
};
