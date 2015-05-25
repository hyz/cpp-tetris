#include <string>
#include <iostream>
#include <boost/msm/back/state_machine.hpp>
#include <boost/msm/front/state_machine_def.hpp>
#include <boost/msm/front/functor_row.hpp>

#define LOG std::clog

namespace msm = boost::msm;
namespace mpl = boost::mpl;
using namespace msm::front;
//using namespace std;
//using namespace std;
//using namespace msm::front::euml; // for And_ operator

struct Ev_Restart {};
struct Ev_Input {
    int a;
    Ev_Input(int x) { a=x; }
};
struct Ev_Back {
    int unpause;
    Ev_Back(int x=0) { unpause=x; }
};
struct Ev_Over {};
struct Ev_Timeout {};
struct Ev_Leave {};
struct Ev_Play {};
struct Ev_Menu {};

struct Ev_Blink {};
struct Ev_EndBlink {};

template <class M, class Ev> void do_event(M& m, Ev const& ev)
{
    static char const* const state_names[] = {"Preview", "Menu", "Playing", "GameOver", "NonPlaying", "YesPlaying", "Paused", "Quit", "Busy", "Blinking", "", "", ""};
    int ix;
    ix = m.current_state()[0];
    LOG << "=B " <<ix<<" "<< state_names[ix] << " Ev:"<< typeid(Ev).name() <<"\n";
    m.process_event(ev);
    ix = m.current_state()[0];
    LOG << "=E " << state_names[ix] <<"\n";
}

struct PrintS
{
    std::string tag;
    PrintS() {}
    PrintS(std::string x) : tag(x) {}
    template <class Ev, class SM, class SS, class TS>
    void operator()(Ev const& ev, SM& sm, SS&, TS&) const
    {
        LOG << tag << " on-ev " << typeid(Ev).name()
            << " state " << typeid(SS).name() <<" "<< typeid(TS).name()
            << "\n";
    }
};

void TheEnd();
struct Main_;
msm::back::state_machine<Main_>& Top();

struct Main_ : msm::front::state_machine_def<Main_> , boost::noncopyable
{
public:
    struct Playing_ : msm::front::state_machine_def<Playing_> // , boost::noncopyable
    {
        struct Action
        {
            template <class Ev, class SM, class SS, class TS>
            void operator()(Ev const& ev, SM&, SS&, TS&) {
            }
        };
        struct AutoFall : Action
        {
            template <class Ev, class SM, class SS, class TS>
            void operator()(Ev const&, SM&, SS&, TS&) {
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
                //do_event(sm, Ev_EndBlink()); // ;
            }
            template <class Ev, class SM> void on_exit(Ev const&, SM& ) {}
        };

        typedef Busy initial_state;
        struct transition_table : mpl::vector<
            Row< Busy     , Ev_Input    , none     , Action    , none >,
            Row< Busy     , Ev_Timeout  , none     , AutoFall  , none >,
            Row< Busy     , Ev_Blink    , Blinking , none      , none >,
            Row< Blinking , Ev_EndBlink , Busy     , none      , none >
        > {};

        template <class Ev, class SM> void on_entry(Ev const& ev, SM&) {
        }
        template <class Ev, class SM> void on_exit(Ev const&, SM& ) {
        }
        template <class SM, class Ev> void no_transition(Ev const&, SM&, int s) {
            LOG << "S:Playing no transition on-ev " << typeid(Ev).name() << "\n";
        }
    }; // Playing_

    struct Play_ : msm::front::state_machine_def<Play_>
    {
        //typedef msm::back::state_machine<Playing_,msm::back::ShallowHistory<mpl::vector<Ev_Resume>>> Playing;
        typedef msm::back::state_machine<Playing_> Playing; // back-end

        struct Preview : msm::front::state<>
        {
            template <class Ev, class SM> void on_entry(Ev const&, SM&) {}
            template <class Ev,class SM> void on_exit(Ev const&, SM&) {}
        }; // Preview

        struct GameOver : msm::front::state<>
        {
            template <class Ev, class SM> void on_entry(Ev const&, SM&) {
            }
            template <class Ev, class SM> void on_exit(Ev const&, SM&) {}
        };

        typedef Preview initial_state;
        struct transition_table : mpl::vector<
            //Row< Preview  , Ev_Leave  , Preview  , PrintS  , none    >,
            Row< Preview  , Ev_Timeout  , Preview  , PrintS  , none    >,
            Row< Preview  , Ev_Restart  , Playing  , PrintS  , none    >,
            //Row< Playing  , Ev_Over     , GameOver , PrintS  , none    >,
            //Row< Playing  , Ev_Restart  , Playing  , PrintS  , none    >,
            Row< GameOver , Ev_Restart  , Preview  , PrintS  , none    >
        > {};

        template <class Ev, class SM> void on_entry(Ev const&, SM& sm) {}
        template <class Ev, class SM> void on_exit(Ev const&, SM& ) {}
        template <class SM, class Ev> void no_transition(Ev const&, SM&, int s) {
            LOG << "S:Play no transition on-ev " << typeid(Ev).name() << "\n";
        }
    };
    typedef msm::back::state_machine<Play_> Play; // back-end
    //typedef msm::back::state_machine<Play_,msm::back::ShallowHistory<mpl::vector<Ev_Play>>> Play;
	//typedef msm::back::state_machine<Play_,msm::back::AlwaysHistory> Play;
	
    struct Leave : msm::front::state<>
    {
        template <class Ev,class SM> void on_entry(Ev const&, SM& sm) {}
        template <class Ev,class SM> void on_exit(Ev const&, SM& ) {}
    }; // Leave

    struct Paused : msm::front::interrupt_state<Ev_Back>
    {
        template <class Ev, class SM> void on_entry(Ev const& ev, SM& top) {
        }
        template <class Ev, class SM> void on_exit(Ev const&, SM& top) {
        }
    }; // Paused
    struct Quit : msm::front::terminate_state<>
    {
        template <class Ev, class SM> void on_entry(Ev const& ev, SM& top) {
        }
        template <class Ev, class SM> void on_exit(Ev const&, SM& top) {
        }
    }; // Quit

    struct PlayX : msm::front::state<> {
        template <class Ev, class SM> void on_entry(Ev const&, SM& sm) {
        }
        template <class Ev, class SM> void on_exit(Ev const&, SM& sm) {
        }
    };

    struct Menu : msm::front::state<>
    {
        template <class Ev,class SM> void on_entry(Ev const&, SM& top) {
        }
        template <class Ev,class SM> void on_exit(Ev const&, SM& top) {
		}
    }; // Menu

    struct isUnpause {
        template <class Ev, class SM, class SS, class TS>
        bool operator()(Ev const& b, SM&, SS&, TS& ) const {return 0; }//{(b.unpause);}
    };
    struct isPlaying {
        template <class Ev, class SM, class SS, class TS>
        bool operator()(Ev const&, SM& sm, SS&, TS& ) const { return 1; }
    };

    typedef mpl::vector<Play,PlayX> initial_state;

    struct transition_table : mpl::vector<
        Row< Play    , Ev_Leave   , Leave   , PrintS  , none        >,
        Row< Play    , boost::any , none    , PrintS  , none        >,
        Row< Leave   , Ev_Play    , Play    , PrintS  , none        >,

        Row< PlayX   , Ev_Menu    , Menu    , PrintS  , none        >,
        Row< PlayX   , Ev_Back    , Quit    , PrintS  , none        >,
        Row< PlayX   , Ev_Back    , Paused  , PrintS  , isPlaying   >,
        Row< Paused  , Ev_Back    , Quit    , PrintS  , none        >,
        Row< Paused  , Ev_Back    , PlayX   , PrintS  , isUnpause   >,
        Row< Menu    , Ev_Back    , PlayX   , PrintS  , none        >
    > {};

    template <class Ev, class SM> void on_entry(Ev const&, SM& sm) {
        LOG << "S:Main entry\n";
    }
    template <class Ev, class SM> void on_exit(Ev const&, SM&) {
        LOG << "S:Main exit\n";
    }
    template <class SM, class Ev> void no_transition(Ev const&, SM&, int state) {
        LOG << "S:Main no transition on-ev " << typeid(Ev).name() << "\n";
    }
}; // Main_

static class App_* app_ = 0;

class App_ : boost::noncopyable
{
public:
    msm::back::state_machine<Main_> main_;

    App_() // : main_(boost::ref(io_service()))
    {
        app_ = this;
        //main_.set_states( back::states_ << Main_::Playing(boost::ref(main_)) );
    }
};

msm::back::state_machine<Main_>& Top()
{
    return app_->main_;
}
void TheEnd()
{
    app_->main_.stop();
    exit(0);//app_->quit();
};

int main()
{
    App_ app; //(boost::ref(io_s));
    auto& top = Top();

    do_event(top, Ev_Leave());
    //do_event(top, Ev_Input(1));
    //do_event(top, Ev_Restart());
    //do_event(top, Ev_Input(0));
//    do_event(top, Ev_Blink());
//    do_event(top, Ev_Back());
//  //do_event(top, Ev_Resume());
//    do_event(top, Ev_Input(1));
//    do_event(top, Ev_EndBlink());
//    do_event(top, Ev_Input(0));
//    do_event(top, Ev_Blink());
//    do_event(top, Ev_Input(1));
//    do_event(top, Ev_EndBlink());
//    do_event(top, Ev_Input(0));
//    do_event(top, Ev_Quit());
//    do_event(top, Ev_Quit());
//  //do_event(top, Ev_Quit());
//
    return 0;
}


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

