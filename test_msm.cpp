#include <iostream>
#include <boost/msm/back/state_machine.hpp>
#include <boost/msm/front/state_machine_def.hpp>
#include <boost/msm/front/functor_row.hpp>

using namespace std;
namespace mpl = boost::mpl;
namespace msm = boost::msm;
using namespace msm::front;
// for And_ operator
//using namespace msm::front::euml;

// State S1 and S2 allow any class1 events
struct class1 {};
// Only state S2 allows class2 events
struct class2 {};

// an upgrade request is a class1 event that requests an upgrade to state 2
struct upgrade_req : public class1 {};

struct MyFSM_ : public msm::front::state_machine_def< MyFSM_ >
{
    /// State 1. Allows any class1 event
    struct S1 : public msm::front::state<>
    {
        template <class Event,class FSM>
        void on_entry(Event const&,FSM& ) {std::cout << "entering: S1" << std::endl;}
        template <class Event,class FSM>
        void on_exit(Event const&,FSM& ) {std::cout << "leaving: S1" << std::endl;}

        /// functor says "processing event in State 1"
        struct Aclass1
        {
            template <class EVT,class FSM,class SourceState,class TargetState>
            void operator()(EVT const& evt ,FSM&,SourceState& ,TargetState& )
            {
                std::cout << "S1:Aclass1\n";
            }
        };

        struct internal_transition_table : mpl::vector<
            //        Event   Action        Guard
            //       +-------+-------------+------------+
            Internal< class1, Aclass1, none >
        > {};
    }; // S1

    /// State 2. Allows any class1 or class2 events
    struct S2 : public msm::front::state<>
    {
        /// functor says "processing event in State 2"
        struct Action
        {
            template <class EVT,class FSM,class SourceState,class TargetState>
            void operator()(EVT const& evt ,FSM&,SourceState& ,TargetState& )
            {
                std::cout << "S2:Action\n";
            }
        };

        struct internal_transition_table : mpl::vector<
            //        Event   Action        Guard
            //       +-------+-------------+------------+
            Internal< class1, Action, none >,
            Internal< class2, Action, none >
        > {};
    }; // S2

    /// everybody starts in state 1
    typedef S1 initial_state;

    /// send an error if a class2 event was received for state1
    struct ASendError { /* ... */
        template <class EVT,class FSM,class SourceState,class TargetState>
        void operator()(EVT const& evt ,FSM&,SourceState& ,TargetState& )
        {
            std::cout << "ASendError\n";
        }
    };

    /// Send a response to the upgrade request
    struct ASendUpgradeRsp { /* ... */
        template <class EVT,class FSM,class SourceState,class TargetState>
        void operator()(EVT const& evt ,FSM&,SourceState& ,TargetState& )
        {
            std::cout << "ASendUpgradeRsp\n";
        }
    };

    /// functor returns true if the request to upgrade to state 2 is OK.
    struct VerifyUpgradeReq
    {
        template <class EVT,class FSM,class SourceState,class TargetState>
        bool operator()(EVT const& evt ,FSM&,SourceState& ,TargetState& )
        {
            std::cout << "VerifyUpgradeReq guard\n";
            return false;
        }
    };

    struct transition_table : mpl::vector<
        //  Start  Event         Next   Action           Guard
        // +------+-------------+------+----------------+------------------+
        Row< S1,   class2,       S1,    ASendError,      none >,
        Row< S1,   upgrade_req,  S2,    ASendUpgradeRsp, VerifyUpgradeReq >,
        Row< S2,   class1,       none,  none,            none >,
        Row< S2,   class2,       none,  none,            none >
        //Row< S1,   class1,       none,  S1::Aclass1,     none >,
    > {};
}; // MyFSM_
typedef msm::back::state_machine<MyFSM_> MyFSM;

void msm_test()
{
    MyFSM fsm;
    fsm.process_event(class1());
    //fsm.process_event(upgrade_req());
}

