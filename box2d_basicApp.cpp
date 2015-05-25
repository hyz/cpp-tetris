#include <string>
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

using namespace ci;
using namespace ci::app;
using namespace std;
using namespace boost::posix_time;
const float BOX_SIZE = 40;

class box2d_basicApp : public AppNative {
  public:
	void prepareSettings( Settings *settings );
	void setup();

	void mouseDown( MouseEvent event );	
	void keyDown( KeyEvent event );
	void update();

	void draw();
	void drawMultiArray(Array2d const& m, bool bg=0);
	void drawStatus();
    void drawScore();
    void drawPreview();
	void play_sound( const char* asset );

	void new_game();
    void pause()
    {
        if (stat_ == stat::normal)
            stat_ = stat::pause;
        else if (stat_ == stat::pause)
            stat_ = stat::normal;
    }
    void move_down(bool keydown)
    {
        if (!M.Move(0)) {
            rounds_.push_back(M.last_round_result);
            if (!M.next_round()) {
                game_over();
            } else if (keydown) {
                play_sound( "speedown.wav" );
            }
        }
    }
	void game_over() {
		stat_= stat::over;
		//play_sound( "gameover.wav" );
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
    int get_level() const
    {
        return rounds_.size()/10; //(get_score()+90) / 100;
    }

	void addBox( const Vec2f &pos );
	b2World				*mWorld;
	vector<b2Body*>		mBoxes;

	Tetris_Basic M;
    std::vector<round_result> rounds_;
    enum class stat {
        normal=0, over=1, pause
    };
	stat stat_;
	unsigned char box_size_;
	
	audio::VoiceRef mVoice;
    //audio::GainNodeRef              mGain;
    //audio::BufferPlayerNodeRef      mBufferPlayerNode;
};

void box2d_basicApp::prepareSettings( Settings *settings )
{
    settings->setWindowSize( BOX_SIZE*16, BOX_SIZE*22 );

	//box_size_ = ;
	//extern void msm_test(); 	msm_test();
}

void box2d_basicApp::setup()
{
	b2Vec2 gravity( 0.0f, 5.0f );
	mWorld = new b2World( gravity );

	b2BodyDef groundBodyDef;
	groundBodyDef.position.Set( 0.0f, getWindowHeight() );
	b2Body* groundBody = mWorld->CreateBody(&groundBodyDef);

	// Define the ground box shape.
	b2PolygonShape groundBox;

	// The extents are the half-widths of the box.
	groundBox.SetAsBox( getWindowWidth(), 10.0f );

	// Add the ground fixture to the ground body.
	groundBody->CreateFixture(&groundBox, 0.0f);

	//auto ctx = audio::Context::master();
	//auto sourceFile = /*, ctx->getSampleRate()*/);//(app::loadResource("1.wav"));
//audio::BufferRef buffer = sourceFile->loadBuffer();
//    mBufferPlayerNode = ctx->makeNode( new audio::BufferPlayerNode( buffer ) );
//    // add a Gain to reduce the volume
//    mGain = ctx->makeNode( new audio::GainNode( 0.5f ) );
//    // connect and enable the Context
//    mBufferPlayerNode >> mGain >> ctx->getOutput();
//    ctx->enable();

	//float volume = 1.0f - (float)event.getPos().y / (float)getWindowHeight();
	//float pan = (float)event.getPos().x / (float)getWindowWidth();
	//mVoice->setVolume( volume );
	//mVoice->setPan( pan );

	new_game();
}
void box2d_basicApp::play_sound( const char* asset )
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

void box2d_basicApp::new_game()
{
    rounds_.clear();;
	stat_ = stat::normal;
	M.reset(20, 10);  //std::cerr << M << "\n";
	M.next_round();
	// play_sound( "newgame.wav" );
}

void box2d_basicApp::addBox( const Vec2f &pos )
{
	b2BodyDef bodyDef;
	bodyDef.type = b2_dynamicBody;
	bodyDef.position.Set( pos.x, pos.y );

	b2Body *body = mWorld->CreateBody( &bodyDef );

	b2PolygonShape dynamicBox;
	dynamicBox.SetAsBox( BOX_SIZE, BOX_SIZE );

	b2FixtureDef fixtureDef;
	fixtureDef.shape = &dynamicBox;
	fixtureDef.density = 1.0f;
	fixtureDef.friction = 0.3f;
	fixtureDef.restitution = 0.5f; // bounce

	body->CreateFixture( &fixtureDef );
	mBoxes.push_back( body );
}

void box2d_basicApp::mouseDown( MouseEvent event )
{
    pause();
	//addBox( event.getPos() );
}

void box2d_basicApp::keyDown( KeyEvent event )
{
#if defined( CINDER_COCOA )
    bool isModDown = event.isMetaDown();
#else // windows
    bool isModDown = event.isControlDown();
#endif
    if (isModDown) {
		if( event.getChar() == 'n' ) {
			new_game();
        }
		return;
	}
    if (event.getCode() == KeyEvent::KEY_ESCAPE || event.getChar() == 'q') {
        // ask confirm
		quit();
		return;
	}
	if (stat_ != stat::normal) {
        if (stat_ == stat::over) {
            if (microsec_clock::local_time() - M.td_ > seconds(3)) {
                new_game();
            }
		} else if (stat_ == stat::pause) {
			if (event.getCode() == KeyEvent::KEY_SPACE) {
				pause();
			}
		}
		return;
    }
	if (event.getCode() == KeyEvent::KEY_SPACE) {
		pause();
		return;
	}
    if (event.getCode() == KeyEvent::KEY_UP) {
		M.rotate();
		play_sound( "rotate.wav" );
    } else if (event.getCode() == KeyEvent::KEY_LEFT) {
		M.Move(-1);
    } else if (event.getCode() == KeyEvent::KEY_RIGHT) {
		M.Move(1);
    } else if (event.getCode() == KeyEvent::KEY_DOWN) {
        move_down(1);
	}
}

void box2d_basicApp::update()
{
	//for( int i = 0; i < 10; ++i ) mWorld->Step( 1 / 30.0f, 10, 10 );

	if (stat_ != stat::normal)
		return;

	if (microsec_clock::local_time() - M.td_ > milliseconds(900 - get_level()*10 - get_score()/8)) {
        move_down(0);
	}
}

void box2d_basicApp::drawMultiArray(Array2d const& m, bool bg)
{
	int bsiz = BOX_SIZE;
	auto const s = get_shape(m);
	for (int y=0; y != s[0]; ++y) {
		for (int x=0; x != s[1]; ++x) {
			Rectf rect((bsiz+1)*x, (bsiz+1)*y, (bsiz+1)*x+bsiz, (bsiz+1)*y+bsiz);
			if (m[y][x]) {
				gl::color( Color( 0.6f, 0.3f, 0.15f ) );
			} else if (bg) {
				gl::color( Color( 0.1f, 0.1f, 0.1f ) );
			} else
				continue;
			gl::drawSolidRect( rect );
		}
	}
}

gl::TextureRef make_tex(std::string const& line)
{
    TextLayout layout;
    layout.setFont( Font( "Arial", 32 ) );
    layout.setColor( Color( 1, 1, 0 ) );

    layout.addLine( line );
    //if (stat_ == stat::over) {
    //    //layout.addLine( std::string("synths: ") );
    //} else if (stat_ == stat::pause) {
    //    layout.addLine( std::string("Pause") );
    //}
    return gl::Texture::create( layout.render( true ) );
}

void box2d_basicApp::drawPreview()
{
    Array2d a(boost::extents[4][4]);
    or_assign(a, make_array(0,0), M.pv_);
    drawMultiArray(a, 1);

    gl::translate(0, (BOX_SIZE+1)*4+10);
    drawScore();
}

void box2d_basicApp::drawScore()
{
	char sbuf[64];
	sprintf(sbuf, "score: %d", get_score()); // std::itoa(100);
    gl::color( Color::white() );
    gl::TextureRef tex = make_tex(sbuf);
    gl::draw( tex, Vec2f(0,0) );
}

void box2d_basicApp::drawStatus()
{
    gl::color( Color::white() );

    if (stat_ != stat::normal) {
		gl::TextureRef tex;
        if (stat_ == stat::over) {
            tex = make_tex("Game over");
        } else if (stat_ == stat::pause) {
            tex = make_tex("Pause");
        }
        Vec2f offset( ((BOX_SIZE+1)*10 - tex->getWidth())/2, 50 );
        gl::draw( tex, offset );
    }
}

void box2d_basicApp::draw()
{
	gl::clear( Color( 0, 0, 0 ) );
	gl::color( Color( 1, 0.5f, 0.25f ) );

	glPushMatrix();
        gl::translate(5, 5);
        drawMultiArray(M.vmat_, 1);
        gl::translate( (BOX_SIZE+1)*M.p_[1], (BOX_SIZE+1)*M.p_[0] );
        drawMultiArray(M.smat_);
	glPopMatrix();

	boost::array<int,2> s = get_shape(M.vmat_);
	glPushMatrix();
		gl::enableAlphaBlending();
        drawStatus();
        gl::translate((BOX_SIZE+1)*10+10, 10);
        drawPreview();
		gl::disableAlphaBlending();
	glPopMatrix();

	//for( vector<b2Body*>::const_iterator boxIt = mBoxes.begin(); boxIt != mBoxes.end(); ++boxIt ) {
	//	Vec2f pos( (*boxIt)->GetPosition().x, (*boxIt)->GetPosition().y );
	//	float t = toDegrees( (*boxIt)->GetAngle() );

	//	glPushMatrix();
	//	gl::translate( pos );
	//	gl::rotate( t );

	//	Rectf rect( -BOX_SIZE, -BOX_SIZE, BOX_SIZE, BOX_SIZE );
	//	gl::drawSolidRect( rect );

	//	glPopMatrix();	
	//}
}

CINDER_APP_NATIVE( box2d_basicApp, RendererGl )

//	void VoiceBasicApp::mouseDown( MouseEvent event )
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
