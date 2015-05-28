#define BOOST_MPL_CFG_NO_PREPROCESSED_HEADERS
#define BOOST_MPL_LIMIT_VECTOR_SIZE 30 //or whatever you need                       
#define BOOST_MPL_LIMIT_MAP_SIZE 30 //or whatever you need                   
#include <string>
#include <iostream>
#include <boost/asio/io_service.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <boost/msm/back/state_machine.hpp>
#include <boost/msm/front/state_machine_def.hpp>
#include <boost/msm/front/functor_row.hpp>

#include <Box2D/Box2D.h>
#include "cinder/app/AppNative.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Texture.h"
#include "cinder/Text.h"
#include "cinder/audio/Voice.h"
//#include "cinder/audio/Context.h"
//#include "cinder/audio/NodeEffects.h"
//#include "cinder/audio/SamplePlayerNode.h"

// #include "cinder/DataSource.h"
#include "multi_array_tetris.hpp"
#define LOG std::clog

using namespace ci;
using namespace ci::app;
using namespace boost::posix_time;

namespace msm = boost::msm;
namespace mpl = boost::mpl;
using namespace msm::front;
//using namespace std;
//using namespace std;
//using namespace msm::front::euml; // for And_ operator

static const int BOX_SIZE = 40;
static const int BOX_SIZEpx = 41;

struct Ev_Input {
    int a;
    Ev_Input(int x) { a=x; }
};
struct Ev_Play {};
struct Ev_Over {};
struct Ev_Back {};
struct Ev_Quit {};
struct Ev_Timeout {};
struct Ev_Menu {};

struct Ev_Blink {};
struct Ev_EndBlink {};

template <class M, class Ev> void process_event(M& m, Ev const& ev)
{
    static char const* const state_names[] = {
        "Preview", "Menu", "Playing", "GameOver", "NonPlaying", "YesPlaying", "Paused", "Quit", "Busy", "Blinking", "", "", ""};
    LOG << "=B " << state_names[m.current_state()[0]] << " Ev:"<< typeid(Ev).name() <<"\n";
    m.process_event(ev);
    LOG << "=E " << state_names[m.current_state()[0]] <<"\n";
}
template <class Ev> void process_event(Ev const& ev)
{
    process_event(Top(), ev);
}

struct Model : Tetris_Basic
{
    //enum class stat { normal=0, over=1, pause };

    std::vector<round_result> rounds_;
    std::string stats;
    Model() {}

    void reset()
    {
        rounds_.clear();;
        // stat_ = stat::normal;

        Tetris_Basic::reset(20, 10);  //std::cerr << model << "\n";
        Tetris_Basic::next_round();
    }

    int get_score() const
    {
        int score = 0;
        for (auto& x : rounds_) {
            switch (x.size()) {
                case 4: score += 40 * 2; break;
                case 3: score += 30 + 15; break;
                case 2: score += 20 + 10; break;
                case 1: score += 10; break;
            }
        }
        return score;
    }
    int get_level() const { return rounds_.size()/10; }

    // (microsec_clock::local_time())
    milliseconds time2falling(int n) const
    {
        //return (tp - td_ > milliseconds(900 - get_level()*10 - get_score()/8));
        return milliseconds(900 - get_level()*10 - get_score()/8);
    }
};

struct View
{
    void operator()(Model const& M/*, std::string const& stats*/);

    Vec2i drawMain(Vec2i bp, Model const& M);
    Vec2i drawSide(Vec2i bp, Model const& M);

    void play_sound( const char* asset );

    gl::TextureRef make_tex(std::string const& line)
    {
        TextLayout layout;
        layout.setFont( Font( "Arial", 32 ) );
        layout.setColor( Color( 1, 1, 0 ) );

        layout.addLine( line );
        return gl::Texture::create( layout.render( true ) );
    }
    inline Vec2i size(Vec2i bp, Vec2i ep) { return Vec2i(abs(ep.x - bp.x), abs(ep.y - bp.y)); }

    Vec2i drawString(int x, int y, std::string const& v);
    Vec2i drawString(Vec2i bp, Vec2i ep, std::string const& v);
	template <typename Pred>
    Vec2i drawArray2d(Vec2i bp, Array2d const& m, Color color, Pred pred);

    void prepareSettings(AppNative::Settings *settings) {
        settings->setWindowSize( BOX_SIZE*16, BOX_SIZE*22 );
    }

    View() : boxfg_(0.6f, 0.3f, 0.15f), boxbg_(0.1f, 0.1f, 0.1f) {}

    Color boxfg_;
    Color boxbg_;
    audio::VoiceRef mVoice;
};

Vec2i View::drawString(int x, int y, std::string const& v)
{
    gl::TextureRef tex = make_tex(v); //("score: " + std::to_string(v));
    gl::draw( tex, Vec2i(x,y) );
    return Vec2i(x,y) + tex->getSize();
}

Vec2i View::drawString(Vec2i bp, Vec2i ep, std::string const& v)
{
    gl::TextureRef tex = make_tex(v); //("score: " + std::to_string(v));

    Vec2i s0 = size(bp, ep);
    Vec2i s1 = tex->getSize();

    int w = s0.x;
    if (s0.x > s1.x) {
        w = (s0.x - s1.x) / 2;
    }
    int h = s0.y;
    if (s0.y > s1.y) {
        h = (s0.y - s1.y) / 2;
    }
    bp += Vec2i(w,h);
    gl::draw( tex, bp );
    return bp + Vec2i(w,h);
}

template <typename Pred>
Vec2i View::drawArray2d(Vec2i bp, Array2d const& m, Color color, Pred pred)
{
    Vec2i endp;
    int bsiz = BOX_SIZE;
    auto const s = get_shape(m);
    gl::color( color );
    for (int y=0; y != s[0]; ++y) {
        for (int x=0; x != s[1]; ++x) {
            if (pred(m[y][x])) {
                Vec2i p( (bsiz+1)*x + bp.x , (bsiz+1)*y + bp.y );
                endp = p + Vec2i(bsiz, bsiz);
                Rectf rect(p, endp);
                // if (m[y][x]) {
                //     gl::color( Color( 0.6f, 0.3f, 0.15f ) );
                // } else if (bg) {
                //     gl::color( Color( 0.1f, 0.1f, 0.1f ) );
                // } //else continue;
                gl::drawSolidRect( rect );
            }
        }
    }
    return endp;
}

struct AlwayTrue{
    template <typename T> int operator()(T const&) const { return 1; }
};
struct IsTrue{
    template <typename T> int operator()(T const& b) const { return (!!b); }
};

Vec2i View::drawMain(Vec2i bp, Model const& M)
{
    Vec2i ep = drawArray2d(bp, M.vmat_, boxbg_, AlwayTrue());

    drawArray2d(bp, M.vmat_, boxfg_, IsTrue());

    Vec2i p( bp.x + BOX_SIZEpx*M.p_[1], bp.y + BOX_SIZEpx*M.p_[0] );
    drawArray2d(p, M.smat_, boxfg_, IsTrue());

    return ep;
}

Vec2i View::drawSide(Vec2i bp, Model const& M)
{
    Vec2i ep;

    auto s = get_shape(M.pv_);
    Array2d v(boost::extents[std::max(4,s[0])][std::max(4,s[1])]);
    ep = drawArray2d(bp, v, boxbg_, AlwayTrue());
    drawArray2d(bp, M.pv_, boxfg_, IsTrue());

    gl::enableAlphaBlending();
    gl::color( Color::white() );
    ep = drawString(bp.x, ep.y+BOX_SIZE, "score: " + std::to_string(M.get_score()));
    ep = drawString(bp.x, ep.y+2, "level: " + std::to_string(M.get_level()));
    ep = drawString(bp.x, ep.y+2, "round: " + std::to_string(M.rounds_.size()));
    gl::disableAlphaBlending();

    return ep;
}

void View::operator()(Model const& M)
{
    gl::clear( Color( 0, 0, 0 ) );
    glPushMatrix();
        // gl::translate(x, y);
        // gl::color( Color( 1, 0.5f, 0.25f ) );

        Vec2i bp(BOX_SIZE,BOX_SIZE);
        Vec2i ep = drawMain(bp, M);
        drawSide(Vec2i(ep.x+BOX_SIZE, bp.y), M);

        if (!M.stats.empty()) {
            gl::enableAlphaBlending();
            gl::color( Color::white() );
            drawString(bp, ep, M.stats); //("Game over"); ("Pause");
            gl::disableAlphaBlending();
        }
    glPopMatrix();
}

void View::play_sound( const char* asset )
{
    fs::path p = "sound";
    try {
        if (mVoice)
            mVoice->stop();
        mVoice = audio::Voice::create( audio::load(loadAsset(p/asset)) );

        float volume = 1.0f;
        float pan = 0.5f;

        mVoice->setVolume( volume );
        mVoice->setPan( pan );

        if( mVoice->isPlaying() )
            mVoice->stop();

        mVoice->start();
    } catch (...) {
    }
}

struct PrintS
{
    template <class Ev, class SM, class SS, class TS>
    void operator()(Ev const& ev, SM& sm, SS&, TS&) const
    {
        LOG << " on-ev " << typeid(Ev).name()
            << " state " << typeid(SS).name() <<" "<< typeid(TS).name()
            << "\n";
    }
};

void The_End();
struct Main_;
msm::back::state_machine<Main_>& Top();

struct Main_ : msm::front::state_machine_def<Main_> , boost::noncopyable
{
    Model model;
    View view;

    boost::asio::deadline_timer deadline_;

public:
    Main_(boost::asio::io_service& io_s) : deadline_(io_s)
    {
    }

    void draw() { view(model); }
    void update() {}
    boost::asio::deadline_timer* deadline_timer() { return &deadline_; }

    struct Default : msm::front::state<>
    {
        template <class Ev, class SM> void on_entry(Ev const&, SM& ) {}
        template <class Ev, class SM> void on_exit(Ev const&, SM& ) {}
    };
    struct Closed : msm::front::state<>
    {
        template <class Ev, class SM> void on_entry(Ev const&, SM& ) {}
        template <class Ev, class SM> void on_exit(Ev const&, SM& ) {}
    };
    struct Back
    {
        template <class Ev, class SM, class SS, class TS>
        void operator()(Ev const& ev, SM&, SS&, TS&) { process_event( Ev_Back() ); }
    };

    struct Menu_ : msm::front::state_machine_def<Menu_>
    {
        typedef Default initial_state;
        struct transition_table : mpl::vector<
            Row< Default  , Ev_Back     , Closed  , Back  , none >
        > {};
        template <class Ev,class SM> void on_entry(Ev const&, SM&) {
			Top().model.stats = "Menu";
		}
        template <class Ev,class SM> void on_exit(Ev const&, SM&) {
			Top().model.stats = "";
		}
    }; // Menu_
    typedef msm::back::state_machine<Menu_> Menu; // back-end

    struct Playing_ : msm::front::state_machine_def<Playing_> // , boost::noncopyable
    {
        int n_reset_;
        struct Action
        {
            template <class Ev, class SM, class SS, class TS>
            void operator()(Ev const& ev, SM& sm, SS&, TS&) {
                auto& top = Top();
                if (is_rotate(ev)) {
                    sm.timer_do(1, boost::system::error_code());
                    top.model.rotate();
                    top.view.play_sound( "rotate.wav" );
                } else if (act(ev, top, sm, "speedown.wav")) {
                } else {
                    process_event(top, Ev_Over());
                    return;
                }
            }
            template <class Top,class SM>
            int act(Ev_Input ev, Top& top, SM& sm, char const* snd)
            {
                if (!top.model.Move(ev.a)) {
                    if (ev.a == 0) {
                        top.model.rounds_.push_back( top.model.last_round_result);
                        if (!top.model.next_round())
                            return 0;
                        if (snd)
                            top.view.play_sound(snd);
                    }
                }
                return 1;
            }
            int is_rotate(Ev_Input ev) { return (ev.a > 1); }
        };
        struct AutoFall : Action
        {
            template <class Ev, class SM, class SS, class TS>
            void operator()(Ev const&, SM& sm, SS&, TS&) {
                act(Ev_Input(0), Top(), sm, 0);
            }
        };
        struct Busy : msm::front::state<>
        {
            template <class Ev, class SM> void on_entry(Ev const&, SM& ) {}
            template <class Ev, class SM> void on_exit(Ev const&, SM& ) {}
        };
        struct Blinking : msm::front::state<>
        {
            template <class Ev, class SM> void on_entry(Ev const&, SM& sm) {
                process_event(sm, Ev_EndBlink()); // ;
            }
            template <class Ev, class SM> void on_exit(Ev const&, SM& ) {}
        };
        struct Paused : msm::front::state<>
        {
            template <class Ev, class SM> void on_entry(Ev const& ev, SM&) {
                Top().model.stats = "Paused";
            }
            template <class Ev, class SM> void on_exit(Ev const&, SM&) {
                Top().model.stats = "";
            }
        }; // Paused

        typedef Busy initial_state;
        struct transition_table : mpl::vector<
            Row< Busy     , Ev_Input    , none     , Action    , none >,
            Row< Busy     , Ev_Timeout  , none     , AutoFall  , none >,
            Row< Busy     , Ev_Blink    , Blinking , none      , none >,
            Row< Busy     , Ev_Back     , Paused   , none      , none >,
	        Row< Busy     , Ev_Menu     , Menu     , none      , none >,
			Row< Menu     , Ev_Back     , Busy     , none      , none >,
            Row< Paused   , Ev_Back     , Busy     , none      , none >,
            Row< Blinking , Ev_EndBlink , Busy     , none      , none >
        > {};

        template <class Ev, class SM> void on_entry(Ev const&, SM&) {
            auto& top = Top();
            top.model.reset();
			timer_ = top.deadline_timer();
            timer_do(1, boost::system::error_code());
        }
        template <class Ev, class SM> void on_exit(Ev const&, SM& ) {
            boost::system::error_code ec;
            timer_->cancel(ec);
            timer_ = 0;
        }
        template <class SM, class Ev> void no_transition(Ev const&, SM&, int) {
            LOG << "S:Playing no transition on-ev " << typeid(Ev).name() << "\n";
        }

        void timer_do(int rs, boost::system::error_code ec)
        {
            if (!timer_ || ec) {
                return;
            }
            if (rs) {
                ++n_reset_;
            } else {
                n_reset_ = 0;
                process_event(Ev_Timeout());
            }
            milliseconds ms = Top().model.time2falling(n_reset_);
            timer_->expires_from_now(ms);
            timer_->async_wait( boost::bind(&Playing_::timer_do, this, 0, _1) );
        }
        boost::asio::deadline_timer* timer_; // std::unique_ptr<boost::asio::deadline_timer>

    }; // Playing_

    struct Play_ : msm::front::state_machine_def<Play_>
    {
        //typedef msm::back::state_machine<Playing_,msm::back::ShallowHistory<mpl::vector<Ev_Resume>>> Playing;
        typedef msm::back::state_machine<Playing_> Playing; // back-end

        struct Preview_ : msm::front::state_machine_def<Preview_>
        {
            typedef Default initial_state;
            struct transition_table : mpl::vector<
                Row< Default  , Ev_Menu     , Menu     , none  , none >,
                Row< Menu     , Ev_Back     , Default  , none  , none >
            > {};
            template <class Ev, class SM> void on_entry(Ev const&, SM&) {
                //Ev_Play
            }
            template <class Ev,class SM> void on_exit(Ev const&, SM&) {}
        }; // Preview_
        typedef msm::back::state_machine<Preview_> Preview; // back-end

        struct Gameover_ : msm::front::state_machine_def<Gameover_>
        {
            typedef Default initial_state;
            struct transition_table : mpl::vector<
                Row< Default  , Ev_Menu     , Menu     , none  , none >,
                Row< Menu     , Ev_Back     , Default  , none  , none >
            > {};
            template <class Ev, class SM> void on_entry(Ev const&, SM&) {}
            template <class Ev,class SM> void on_exit(Ev const&, SM&) {}
        }; // Gameover_
        typedef msm::back::state_machine<Gameover_> GameOver; // back-end

        typedef Preview initial_state;
        struct transition_table : mpl::vector<
            Row< Preview  , Ev_Back     , Closed   , Back  , none    >,
            Row< Playing  , Ev_Back     , Closed   , Back  , none    >,
            Row< GameOver , Ev_Back     , Closed   , Back  , none    >,
            Row< Preview  , Ev_Play     , Playing  , none  , none    >,
            Row< Playing  , Ev_Play     , Playing  , none  , none    >,
            Row< GameOver , Ev_Play     , Playing  , none  , none    >,
            Row< Playing  , Ev_Over     , GameOver , none  , none    >
        > {};

        template <class Ev, class SM> void on_entry(Ev const&, SM& sm) {}
        template <class Ev, class SM> void on_exit(Ev const&, SM& ) {}
        template <class SM, class Ev> void no_transition(Ev const&, SM&, int s) {
            LOG << "S:Play no transition on-ev " << typeid(Ev).name() << "\n";
        }
    }; // Play_
    typedef msm::back::state_machine<Play_> Play; // back-end
    //typedef msm::back::state_machine<Play_,msm::back::ShallowHistory<mpl::vector<Ev_Play>>> Play;
	//typedef msm::back::state_machine<Play_,msm::back::AlwaysHistory> Play;
	
    struct Quit : msm::front::terminate_state<>
    {
        template <class Ev, class SM> void on_entry(Ev const& ev, SM& top) {
            top.io_service().post(&The_End);
        }
        template <class Ev, class SM> void on_exit(Ev const&, SM& top) {
        }
    }; // Quit

    typedef Play initial_state;

    struct transition_table : mpl::vector<
	    Row< Play   , Ev_Quit    , Quit  , none  , none >,
        Row< Play   , Ev_Back    , Quit  , none  , none >
    > {};

    template <class Ev, class SM> void on_entry(Ev const&, SM& sm) {
        LOG << "S:Main entry\n";
    }
    template <class Ev, class SM> void on_exit(Ev const&, SM&) {
        boost::system::error_code ec;
        deadline_.cancel(ec);
        LOG << "S:Main exit\n";
    }
    template <class SM, class Ev> void no_transition(Ev const&, SM&, int state) {
        LOG << "S:Main no transition on-ev " << typeid(Ev).name() << "\n";
    }
	boost::asio::io_service& io_service() { return deadline_.get_io_service(); }
}; // Main_

static class App_* app_ = 0;

class App_ : public AppNative, boost::noncopyable
{
public:
    msm::back::state_machine<Main_> main_;

    App_() : main_(boost::ref(io_service()))
    {
        app_ = this;
        //main_.set_states( back::states_ << Main_::Playing(boost::ref(main_)) );
    }

    void prepareSettings(Settings *settings) { main_.view.prepareSettings(settings); }
    void setup() { main_.start(); } // state_machine:start
    void update() { main_.update(); }
    void draw() { main_.draw(); }

    void mouseDown( MouseEvent event ) { process_event(main_, Ev_Menu()); }
    void keyDown( KeyEvent event );
};

msm::back::state_machine<Main_>& Top()
{
    return app_->main_;
}
void The_End()
{
    app_->main_.stop();
    app_->quit();
};

void App_::keyDown( KeyEvent event )
{
#if defined( CINDER_COCOA )
    bool isModDown = event.isMetaDown();
#else // windows
    bool isModDown = event.isControlDown();
#endif

    if (isModDown) {
        switch ( event.getChar() ){
			case 'n': process_event(main_, Ev_Play()); break;
		    case 'c': process_event(main_, Ev_Quit()); break;
        }
		return;
    }

    int ev = 0;
    switch (event.getCode()) {
        case KeyEvent::KEY_ESCAPE: process_event(main_, Ev_Back()); return;
        case KeyEvent::KEY_SPACE: process_event(main_, Ev_Menu()); return;
        case KeyEvent::KEY_UP: ev = 2; break;
        case KeyEvent::KEY_LEFT: ev = -1; break;
        case KeyEvent::KEY_RIGHT: ev = 1; break;
        case KeyEvent::KEY_DOWN: ev = 0; break;
        default: return;
    }
    process_event(main_, Ev_Input(ev));
}

//int _testmain()
//{
//    boost::asio::io_service io_s;
//    Main te(boost::ref(io_s));
//
//    process_event(te, Ev_Input(1));
//    process_event(te, Ev_Play());
//    process_event(te, Ev_Input(0));
//    process_event(te, Ev_Blink());
//    process_event(te, Ev_Back());
//  //process_event(te, Ev_Resume());
//    process_event(te, Ev_Input(1));
//    process_event(te, Ev_EndBlink());
//    process_event(te, Ev_Input(0));
//    process_event(te, Ev_Blink());
//    process_event(te, Ev_Input(1));
//    process_event(te, Ev_EndBlink());
//    process_event(te, Ev_Input(0));
//    process_event(te, Ev_Quit());
//    process_event(te, Ev_Quit());
//  //process_event(te, Ev_Quit());
//
//    return 0;
//}


//    void VoiceBasicApp::mouseDown( MouseEvent event )
//{
//// scale volume and pan from window coordinates to 0:1
//float volume = 1.0f - (float)event.getPos().y / (float)getWindowHeight();
//float pan = (float)event.getPos().x / (float)getWindowWidth();
//mVoice->setVolume( volume );
//mVoice->setPan( pan );
//// By stopping the Voice first if it is already playing, start will play from the beginning
//if( mVoice->isPlaying() )
//mVoice->stop();
//mVoice->start();
//}
//void VoiceBasicApp::keyDown( KeyEvent event )
//{
//// space toggles the voice between playing and pausing
//if( event.getCode() == KeyEvent::KEY_SPACE ) {
//if( mVoice->isPlaying() )
//mVoice->pause();
//else
//mVoice->start();
//}
//}

CINDER_APP_NATIVE( App_, RendererGl )

