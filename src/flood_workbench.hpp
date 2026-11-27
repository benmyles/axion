class MaterialInterview {
    Session &s;
    grounding::GroundedSpeech &visual;
    bool spoken_work = false;
    grounding::Retina prior{};
    AgentImage &subject, &other;
    DevicePtr eye, listener_eye;
    Temporary samples, lived, law, vocabulary, field_reply, channel, recipient_board, recipient_reply;
    Function inspect_samples{}, receive_call{};
    Temporary saline_cortex, request_note;
    bool field_open = false, saline_table = false;
    Word rehearsal = 1, water_vision = 1, liquid = 0, consult_round = 0, automatic = 0;
    brine::Trace entry_trace{};
#include "flood_brine_talk.hpp"
    Word hearing = 1;
#include "flood_field_talk.hpp"
    Temporary worker, peer, rack, reading, pointed;
    Function work{}, shore_visit{};
    Temporary shore, shore_report;
    bool shore_open = false, shore_requests = false, named_shore = false;
    grounding::Retina shore_prior{};
#include "flood_shore_talk.hpp"
    bool open = false;
    Word span = 0;
    static Word quantity(std::string_view word, Word lo, Word hi) {
        Word n = 0;
        auto [end, error] = std::from_chars(word.data(), word.data() + word.size(), n);
        if (error != std::errc{} || end != word.data() + word.size() || n < lo || n > hi)
            throw std::invalid_argument("invalid mineral workshop quantity");
        return n;
    }

  public:
    MaterialInterview(Session &lab, AgentImage &p, AgentImage &q, DevicePtr image, DevicePtr audience,
                      grounding::GroundedSpeech &view)
        : s(lab), visual(view), subject(p), other(q), eye(image), listener_eye(audience),
          samples(s.d, p.investigates ? sizeof(material::Samples) : 0),
          lived(s.d, p.investigates ? sizeof(material::Inquiry) : 0),
          law(s.d, p.investigates ? sizeof(material::Genes) : 0),
          vocabulary(s.d, p.investigates ? sizeof(material::FieldLanguage) : 0),
          field_reply(s.d, p.investigates ? sizeof(material::FieldAnswer) : 0),
          channel(s.d, p.investigates ? sizeof(Count) : 0),
          recipient_board(s.d, p.investigates ? sizeof(material::ListenerInput) : 0),
          recipient_reply(s.d, p.investigates ? sizeof(material::ListenerResult) : 0),
          saline_cortex(s.d, p.saline ? sizeof(brine::Animal) : 0),
          request_note(s.d, p.saline ? sizeof(material::SampleRequest) : 0), entry_trace(p.brine_mind.cortex.memory),
          worker(s.d, p.handling ? sizeof(material::Worker) : 0), peer(s.d, p.handling ? sizeof(material::Worker) : 0),
          rack(s.d, p.handling ? sizeof(material::SessionState) : 0),
          reading(s.d, p.handling ? sizeof(material::Reading) : 0), pointed(s.d, p.handling ? sizeof(Count) : 0),
          shore(s.d, p.handling ? sizeof(material::ShoreTray) : 0),
          shore_report(s.d, p.handling ? sizeof(material::ShoreReply) : 0) {
        if (p.saline)
            s.d.check(s.d.upload(saline_cortex.ptr, &p.brine_mind.cortex, sizeof(brine::Animal)),
                      "restore individual brine tissue");
    }
    bool control(const std::string &line) {
        if (!worker.ptr || line.empty())
            return false;
        if (samples.ptr && field_control(line))
            return true;
        if (shore_open && (line == "/shore-requests on" || line == "/shore-requests off")) {
            shore_requests = line.ends_with(" on");
            std::cout << "shore_requests=" << shore_requests << std::endl;
            return true;
        }
        if (shore_open && shore_requests && line.front() != '/') {
            named_shore = false;
            bool one = std::all_of(line.begin(), line.end(),
                                   [](unsigned char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'); });
            if (one)
                shore_control("/build-for " + line);
            else
                shore_control("/build-answer " + line);
            if (named_shore)
                shore_control("/pursue-shore");
            return true;
        }
        if (line.front() == '/' && shore_control(line))
            return true;
        if (line == "/work-requests on" || line == "/work-requests off") {
            spoken_work = line.ends_with(" on");
            std::cout << "spoken_work=" << spoken_work << std::endl;
            return true;
        }
        if (open && line.starts_with("/act-on ")) {
            std::string answer;
            if (!eye || !visual.hear(line.substr(8), answer) || answer.empty() || answer == "?" ||
                answer.ends_with("?")) {
                std::cout << "work_request=unresolved" << std::endl;
                return true;
            }
            std::cout << "animal> " << answer << std::endl;
            return control("/work-word " + answer);
        }
        if (line.front() != '/') {
            if (!open || !spoken_work || line.size() > 12 ||
                !std::all_of(line.begin(), line.end(),
                             [](unsigned char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'); }))
                return false;
            std::string lower = line;
            for (char &c : lower)
                if (c >= 'A' && c <= 'Z')
                    c = char(c - 'A' + 'a');
            return control("/work-word " + lower);
        }
        std::istringstream words(line);
        std::string cmd;
        words >> cmd;
        const std::array<std::string_view, 14> commands{
            "/workbench", "/handle-scraps", "/peer-practice", "/move-muscle", "/show-work",
            "/try-work",  "/sun",           "/reach-piece",   "/work-word",   "/inspect-work",
            "/peer-sun",  "/supply",        "/take-grain",    "/return-grain"};
        auto it = std::find(commands.begin(), commands.end(), cmd);
        if (it == commands.end())
            return false;
        Word action = it - commands.begin(), value = 0, amount = 1;
        std::vector<std::string> args;
        for (std::string v; words >> v;)
            args.push_back(v);
        if (action == 0) {
            if (args.empty() || args.size() > 2)
                throw std::invalid_argument("use /workbench SEED [SPAN]");
            value = quantity(args[0], 0, ~0u);
            span = args.size() > 1 ? quantity(args[1], 1, 4) : 2;
            amount = span;
            field_open = false;
            shore_open = false;
            shore_requests = false;
            if (!open) {
                s.bind(work, "flood_workbench", Image::floodplain);
                s.d.check(s.d.upload(worker.ptr, &subject.worker, sizeof(subject.worker)), "seat the original builder");
                auto guest = other.handling ? other.worker : material::Worker{};
                if (!other.handling)
                    guest.genes = material::founder(mix(other.brain.seed + 17919));
                s.d.check(s.d.upload(peer.ptr, &guest, sizeof(guest)), "seat a separate witness");
                open = true;
            }
        } else {
            if (!open) {
                std::cout << "Open the material tray with /workbench SEED." << std::endl;
                return true;
            }
            if (action == 1 || action == 2 || action == 6 || action == 10) {
                if (args.size() != 1)
                    throw std::invalid_argument("specify the number of ticks");
                amount = quantity(args[0], 1, 4096);
            }
            if (action == 3 || action == 4) {
                if (args.size() != 1)
                    throw std::invalid_argument("specify a muscle (0..7) or peer");
                value = (action == 4 && args[0] == "peer") ? 8 : quantity(args[0], 0, 7);
            }
            if (action == 7) {
                if (args.size() != 1)
                    throw std::invalid_argument("specify the socket");
                value = quantity(args[0], 0, span);
            }
            if (action == 11) {
                if (args.size() != 1)
                    throw std::invalid_argument("use /supply SEED");
                value = quantity(args[0], 0, ~0u);
            }
            if (action == 5 || action == 9 || action == 12 || action == 13)
                if (!args.empty())
                    throw std::invalid_argument("this workbench gesture takes no arguments");
            if (action == 8) {
                if (args.size() != 1 || !eye)
                    throw std::invalid_argument("a named workpiece needs one word and an acquired visual circuit");
                Count heard = acoustic::pulse(args[0]);
                s.d.check(s.d.upload(pointed.ptr, &heard, sizeof(heard)), "hear the workpiece name");
            }
        }
        Word reserve = subject.resident.animal.energy,
             peer_reserve = other.handling ? other.resident.animal.energy : 65535;
        void *a[]{&worker.ptr, &peer.ptr, &rack.ptr, &reading.ptr, &eye,         &pointed.ptr,
                  &action,     &value,    &amount,   &reserve,     &peer_reserve};
        s.d.check(s.d.launch(work, 1, 1, 1, 1, 1, 1, 0, nullptr, a, nullptr), "operate the physical mineral workshop");
        material::Reading r;
        s.d.check(s.d.download(&r, reading.ptr, sizeof(r)), "read the material contacts");
        s.d.check(s.d.download(&subject.worker, worker.ptr, sizeof(subject.worker)), "retain the builder's contacts");
        if (other.handling)
            s.d.check(s.d.download(&other.worker, peer.ptr, sizeof(other.worker)), "retain the witness's contacts");
        if (action == 3 || action == 5 || action == 8)
            subject.resident.animal.energy -= r.spent;
        if (action == 4 && other.handling)
            other.resident.animal.energy -= r.spent;
        if (eye && (action == 0 || action == 3 || action == 5 || action == 6 || action == 8 || action == 9)) {
            grounding::View scene{};
            scene.before = prior;
            for (Word j = 0; j <= span + 1; ++j)
                if (r.ports[j])
                    grounding::illuminate(scene.after, j, r.spectra[j]);
            if (action == 0)
                scene.before = scene.after;
            visual.present(scene);
            prior = scene.after;
        }
        if (r.ambiguous)
            std::cout << "work_word=unresolved  form=" << r.form << "  possible=" << r.candidates
                      << "  certain=" << r.certain << std::endl;
        else
            std::cout << "workbench_event=" << cmd.substr(1) << "  operation=" << r.operation
                      << "  visual_change=" << r.changed << "  bonds=" << r.topology << "  connected=" << r.reached
                      << "  reactions=" << r.product << "  fuel_light=" << r.sun << "  reach=" << r.contact
                      << "  named_site=" << (action == 8 ? int(r.site) : -1) << std::endl;
        if (action == 9 || action == 11 || action == 12 || action == 13) {
            std::cout << "workpieces=";
            for (Word i = 0; i <= span + 1; ++i) {
                if (i)
                    std::cout << ',';
                std::cout << i << '/' << r.ports[i] << '/' << r.stored[i] << '/' << r.spectra[i];
            }
            std::cout << "  supply=" << r.stock << "  carried_faces=" << r.cargo << std::endl;
        }
        if (auto *j = journal::current()) {
            j->metrics["workbench_events"]++;
            j->metrics["workbench_reactions"] += r.product;
            j->metrics["workbench_witnessed"] += Word(action == 4 && r.changed);
            j->metrics["workbench_named"] += Word(action == 8 && !r.ambiguous);
        }
        return true;
    }
};
