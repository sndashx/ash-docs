#pragma once
/// Phase 11: dialogue screen (NPC portrait, greeting text, topic list).
#include <functional>
#include <string>
#include <vector>

#include "ui/screen.hpp"

namespace ash {
namespace ui {

/// One conversation topic the player can pick.
struct DialogueTopic {
    std::string                              text;
    std::function<void(ScreenContext&)>       on_select;
};

class DialogueScreen : public Screen {
public:
    using TopicCallback = std::function<void(int /*index*/, ScreenContext&)>;

    DialogueScreen(std::string npc_name,
                   std::string greeting,
                   std::vector<DialogueTopic> topics,
                   TopicCallback on_topic = nullptr);

    void render(render::Buffer& buf, Rect viewport) override;
    ScreenResult handle_key(KeyEvent const& ev, ScreenContext& ctx) override;
    std::string title() const override { return "dialogue"; }

private:
    std::string               npc_name_;
    std::string               greeting_;
    std::vector<DialogueTopic> topics_;
    TopicCallback             on_topic_;
    int                       selected_{0};
    int                       scroll_{0};
    int                       anim_t_ms_{0};

    void update(int dt_ms) override { anim_t_ms_ += dt_ms; }
};

}  // namespace ui
}  // namespace ash