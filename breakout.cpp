#define OLC_PGE_APPLICATION
#include "olcPixelGameEngine.h"
#include "levels.h"

#define ROWS 12
#define COLS 21

#define WIDTH 512
#define HEIGHT 512

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

#define ANIM_DUR 1.5

#define MAX_BALL_SPEED 700
#define MAX_PAD_SPEED 450

// ----- Struct definitions -----
struct Player {
	short lives = 3;
	olc::vf2d pos;
	short speed = 200;
	float length = 100;
	bool shooting_ball = true;	  // says if player is shooting ball, so waits for the input to shoot it 
	bool auto_pilot = true;		// used in title screen for cool effect, follows ball
	bool fail_transition = false; // used in transition when ball goes out of the screen
	bool can_control = true; // says if player can move, useful in transitions
	int rand_off_apilot = (random() % 60) + 20;
	float powerup_timer = 0; // timer that changes to default when 0, otherwise use powerup value
};

struct Ball {
	olc::vf2d pos;
	float speed = 0;
	float saved_speed = 0;
	short size = 7;
	olc::vf2d dir;
	olc::Pixel col = olc::WHITE;
	float powerup_timer = 0; // timer that changes to default when 0, otherwise use powerup value
	bool speedup_or_down = false;
};

struct Level {
	unsigned char bricks[ROWS][COLS]; // Brick data, if they are there or not
	olc::Pixel colors[ROWS][COLS]; // Colors for brick to make game appeal more
};
// ---------------------------------

// ---- Globals -----
	bool game_over = false; 
	long unsigned int score = 0;
	long int level_score = 0;
	uint32_t curr_level = 0;
	const olc::Pixel brick_colors[6] { // we start from 1 btw, because 0 is empty, doesn't render at all
			olc::GREY,		 // WALL
			olc::MAGENTA, 	 // Length UP, and purple
			olc::CYAN, 	    // Length Down
			olc::GREEN,	    // Speed UP
			olc::RED, 		 // Speed Down
			olc::VERY_DARK_RED // Explosive
	};
	
	struct data4fail_animation { float player_x,player_len,ball_x,ball_y;};
	data4fail_animation saved_fail;
	bool sad_face_draw_easter_egg_never_find_free_HD = false;
	long unsigned int fancy_game_over_counter = 0;
	float game_over_counter = 0;
//-------------------

class useful_methods : public olc::PixelGameEngine {
public:

	olc::vf2d normalize(const olc::vf2d& v){
		olc::vf2d copy = v;
		float l = copy.x * copy.x + copy.y * copy.y;
		if (l != 0) {
			l = sqrtf(l);
			copy.x /= l;
			copy.y /= l;
		}
		return copy;
	}
	
	float lerp(const float start,const float end,const float pct){ return (start + (end - start) * pct);}
	float EaseIn(const float t){ return t * t; }
	float Flip(const float x)  { return 1 - x; }
	float EaseOut(const float t) { return Flip((1 - t) * (1 - t) * (1 - t) * (1 - t) * (1 - t)); } // very good easings

	float mapRange(float a1,float a2,float b1,float b2,float s) {return b1 + (s-a1)*(b2-b1)/(a2-a1);}

	void HSVtoRGB(short H, short S,short V, short& R, short& G, short& B){
		H = H % 360;
		S = S % 101;
		V = V % 101;
		
		float s = (float)S/100;
		float v = (float)V/100;
		float C = s*v;
		float X = C*(1-fabs(fmod((float)H/60.0, 2)-1));
		float m = v-C;
		float r,g,b;
		
		if	 (H >= 0   && H < 60)  r = C,g = X,b = 0;
		else if(H >= 60  && H < 120) r = X,g = C,b = 0;
		else if(H >= 120 && H < 180) r = 0,g = C,b = X;
		else if(H >= 180 && H < 240) r = 0,g = X,b = C;
		else if(H >= 240 && H < 300) r = X,g = 0,b = C;
		else r = C,g = 0,b = X;
		
		R = (r+m)*255;
		G = (g+m)*255;
		B = (b+m)*255;
	}
	
	float EaseCircInOut(float t, float b, float c, float d){ // easing stolen from raylib/extras/easings.h :P
		if ((t/=d/2.0f) < 1.0f) return (-c/2.0f*(sqrtf(1.0f - t*t) - 1.0f) + b);
		t -= 2.0f; return (c/2.0f*(sqrtf(1.0f - t*t) + 1.0f) + b);
	}	
	
	float EaseCubicInOut(float t, float b, float c, float d){ // same
		if ((t/=d/2.0f) < 1.0f) return (c/2.0f*t*t*t + b);
		t -= 2.0f; return (c/2.0f*(t*t*t + 2.0f) + b);
	}	
	
	/*
	void DownloadLevel(Level& l, const olc::Sprite* sp){
		short r,g,b;
		auto pixel_to_type = [](olc::Pixel p) { return (p.r == 255)*4 + (p.g == 255)*2 + (p.b == 255); };
		
		for(short i = 0; i < ROWS;++i){
			for(short j = 0; j < COLS;++j){
				l.bricks[i][j] = pixel_to_type(sp->GetPixel(j,i));
				if(l.bricks[i][j] == BASIC){
					HSVtoRGB(mapRange(0,ROWS,380,0,i),60,80,r,g,b);
					l.colors[i][j] = olc::Pixel(r,g,b);
				}
				else if(l.bricks[i][j] != NOTHING){
					//printf("%d\n",l.bricks[i][j]);
					l.colors[i][j] = brick_colors[l.bricks[i][j]- 1];
				}
			}
		}
	}
	*/
	
	void DownloadLevel(Level& l, const std::bitset<12*21*3>* layout){
		short r,g,b;
		
		for(short i = 0; i < ROWS;++i){
			for(short j = 0; j < COLS;++j){
				l.bricks[i][j] = (layout->test((j+(i*COLS))* 3) + layout->test((j+(i*COLS))* 3 + 1)*2 + layout->test((j+(i*COLS))* 3 + 2)*4);
				if(l.bricks[i][j] == BASIC){
					HSVtoRGB(mapRange(0,ROWS,380,0,i),60,80,r,g,b);
					l.colors[i][j] = olc::Pixel(r,g,b);
				}
				else if(l.bricks[i][j] != NOTHING){
					l.colors[i][j] = brick_colors[l.bricks[i][j]- 1];
				}
			}
		}
	}
	
	void ResetBall(Ball& b){
		b.speed = 0;
		b.dir = {-0.6,-0.2};
	}
	
	void ResetPlayer(Player& p){
		p.shooting_ball = true;
		p.speed = 170;
		if(!p.auto_pilot) p.fail_transition = true;
	}
	
	void Transition(const uint8_t& layer,const float final_value,float& curr_time){
		float ease = EaseCircInOut(curr_time,0,final_value, ANIM_DUR);
		if(final_value <= 1)	SetLayerOffset(layer,0,ease);
		else					SetLayerTint(layer,olc::Pixel(ease,ease,ease));
	}
	
	void FailTransition(float final_value,const float start_val,float& output,const float curr_time){
			const float ease = EaseCubicInOut(curr_time,start_val,final_value,2.0);
			output = ease;
	}
	
	void DrawLevel(const Level& l){
		for (short i = 0; i < ROWS; ++i)
			for (short j = 0; j < COLS; ++j){
				if(l.bricks[i][j] != NOTHING) FillRect(j*24 + 6,i*16 + 18,20,10,l.colors[i][j]);
			}
	}

	void DrawPad(const olc::vf2d& pos,const int& size,const olc::Pixel col = olc::WHITE){
		FillCircle(pos.x + size,pos.y + 6,6,col);
		FillCircle(pos.x,pos.y + 6,6,col);
		FillRect(pos.x,pos.y,size,13,col);
	}
	
	void DrawUI(const Player& player){
		if(GetKey(olc::TAB).bHeld){
			DrawString(80,2,"level score", olc::DARK_GREY, 1);
			DrawString(330,2,"current level",olc::DARK_GREY, 1);
			DrawString(455,2,"lives", olc::DARK_GREY, 1);
		}
		DrawString(30,10,std::to_string(level_score),olc::WHITE,4);
		DrawString(360,10,std::to_string(curr_level),olc::WHITE,4);
		DrawString(460,10,std::to_string(player.lives),olc::WHITE,4);
	}
	
	void Fail(Player& p,Ball& b){
		if(curr_level == 0){ sad_face_draw_easter_egg_never_find_free_HD = true; return;}
		if(p.lives == -1){ game_over = true; return;} // if player has no more lives, game_over 
		ResetPlayer(p);
		ResetBall(b);
		p.can_control = false;
		p.lives -= 1;
		level_score -= 100;
		if(saved_fail.player_x == 0){ // idk, it gets fucked up by another thread without this
			saved_fail.player_x = p.pos.x;
			saved_fail.player_len = p.length;
			
			saved_fail.ball_x = b.pos.x;
			saved_fail.ball_y = b.pos.y;
		}
	};
	
	void ExplodeBricks(int y,int x, Level& level) {
		for(int i=y-1; i <= y + 1; ++i){
			for(int j=x-1; j <= x + 1; ++j){
				if(j >= COLS || j < 0 || i < 0 || i >= ROWS) continue;
				if((i == y && j == x)) level.bricks[i][j] = NOTHING;
				
				if(level.bricks[i][j] == EXPLOSIVE) ExplodeBricks(i,j,level); // found another explosive on way, call explosive on them too
				
				level.bricks[i][j] = NOTHING;
				level_score += level.bricks[i][j] == BASIC ? 20 : 100;
			}
		}
	}
	
	void MoveBall(Ball& b,Level& l,Player& p,const float& delta){
		// Move ball
		b.pos += (normalize(b.dir) * b.speed * delta);
		
		// ------- Collision detection -------
		
		//Walls	
		if((b.pos.x > WIDTH-b.size && b.dir.x > 0) || (b.pos.x < b.size && b.dir.x < 0)) b.dir.x *= -1;
		if(b.pos.y < b.size && b.dir.y < 0) b.dir.y *= -1;
		if(b.pos.y > HEIGHT+10){ Fail(p,b); return; }
		
		//Bricks
		const olc::vf2d ball_rect = {b.pos.x - b.size, b.pos.y - b.size};
		for(short i=0;i<ROWS;++i)
			for(short j=0;j<COLS;++j){
				const int brick_type = l.bricks[i][j];
				if(!brick_type) continue;
				//     x         y      w   h
				// j*24 + 6, i*16 + 18, 20, 10
				const olc::vf2d brick_pos = {(float)j*24 + 6,(float)i*16 + 18};
				
				//Draw(brick_pos.x,brick_pos.y,olc::RED);
				//Draw(ball_rect.x,ball_rect.y,olc::GREEN);
				
				if( // Collision occured
					brick_pos.x + 20 >= ball_rect.x &&
					brick_pos.y + 10 >= ball_rect.y &&
					ball_rect.x + b.size*2 >= brick_pos.x &&
					ball_rect.y + b.size*2 >= brick_pos.y
				)	
				{ 
					if(brick_type == LEN_UP || brick_type == LEN_DOWN){
						if(p.powerup_timer <= 0){ 
							p.length = brick_type == LEN_UP ? 200 : 70;
							p.powerup_timer = 10; // timer set to 10 seconds
						}
						level_score += 80;
					}
					else if(brick_type == SPEED_UP || brick_type == SPEED_DOWN){
						if(b.powerup_timer <= 0 || b.speedup_or_down != (brick_type == SPEED_UP)){
							b.saved_speed = b.speed;
							b.speedup_or_down = brick_type == SPEED_UP;
							b.speed += brick_type == SPEED_UP ? 100 : -100;
							b.col    = brick_type == SPEED_UP ? olc::GREEN : olc::RED;
							b.powerup_timer = 7; // timer to 7 seconds
						}
						level_score += 80;
					}
					else if(brick_type == EXPLOSIVE){ // lets destroy the neighbhours
						ExplodeBricks(i,j,l);
					}
					else if(brick_type != WALL) level_score += 20;
					
					if(brick_type != WALL) l.bricks[i][j] = 0;
					if(brick_type == BASIC || brick_type == WALL){
						if(b.pos.x >= brick_pos.x && brick_pos.x + 20 >= b.pos.x) b.dir.y *= -1;
						else if(b.pos.y >= brick_pos.y && brick_pos.y + 10 >= b.pos.y) b.dir.x *= -1;
						
						else{ // I know... it looks bad, but trust me it looks better in game then
							if(b.pos.y > brick_pos.y + 10) b.dir.y = 0.16;
							if(b.pos.y < brick_pos.y) 	 b.dir.y = -0.65;
							//b.dir.y = (0.45+angle_offset) * (1 + -2 * (b.pos.y < brick_pos.y));
							
							//if(b.pos.x > brick_pos.x + 20) b.dir.x = 0.45;
							//if(b.pos.x < brick_pos.x) 	 b.dir.x = -0.45;
							b.dir.x = (0.45) * (1 + -2 * (b.pos.x < brick_pos.x));
						}
					}
					break;
				}
			}
		
		//Pad
		const olc::vf2d pad_pos = p.pos;
		
		if( // Collision occured
			pad_pos.x + p.length + 6  >= ball_rect.x &&
			pad_pos.y + 13 		>= ball_rect.y &&
			ball_rect.x + b.size*2 >= pad_pos.x - 6 &&
			ball_rect.y + b.size*2 >= pad_pos.y &&
			b.pos.y < 450 + b.size && b.dir.y > 0
		)
		{
			p.rand_off_apilot = (random() % 50) + 20; // change random offset from center in case of autopilot
			
			b.dir.y *= -1;
			
			//NOTE lambda to get distance between two points
			auto distance_lambda = [](float x1,float y1,float x2,float y2) { return sqrtf((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1)); };
  
			float distance = distance_lambda(p.pos.x + (p.length /2), p.pos.y + 7.5, b.pos.x,b.pos.y);
			float magnitude = distance - (b.size >> 1) - (13 >> 1);
			float ratio = magnitude / (p.length /2 ) * 2.6;
			
			if(b.pos.x < p.pos.x + (p.length / 2)) {
				ratio = -ratio;
			}
				
			b.speed = MIN(b.speed + (p.auto_pilot ? 2 : 8), MAX_BALL_SPEED + ((b.powerup_timer > 0) * 100));
			p.speed = MIN(p.speed + (p.auto_pilot ? 0 : 8), MAX_PAD_SPEED);
			
			b.dir.x = b.speed * ratio * delta;
		}
		
	}
	
	bool isLevelFinished(const Level& level){
		int bricks_left = 0;
		for(int i=0; i < ROWS; ++i)
			for(int j=0; j < COLS; ++j)
				bricks_left += (level.bricks[i][j] != WALL && level.bricks[i][j]); // we count all bricks that are not walls and are not empty
		return bricks_left == 0;
	}
};

class Game : public useful_methods
{
	Player player;
	Ball ball;

	Level levels_data[LEVEL_COUNT];
	
	// Helpers
	float sec_timer = 0;
	bool show_menu_text = true;
	bool in_menu = true;
	
	bool fade_or_move = false; // false - move; true - fade
	bool in_transition = false;
	float curr_time_anim = 0;
	
	uint32_t game_layer;

public:
	Game()	{ sAppName = "Breakout"; }

public:
	bool OnUserCreate() override
	{
		ball.dir = {-0.60,-0.2};
		ball.speed = 0;
		player.pos = {200,450};
		
		ResetPlayer(player);
		game_layer = CreateLayer();
		EnableLayer(game_layer,true);
		DownloadLevel(levels_data[curr_level],&level_layout[curr_level]);
		return true;
	}

	bool OnUserUpdate(float delta) override
	{
		SetDrawTarget(game_layer); // drawing to game_layer
		Clear(olc::BLACK); 		// clear it to Black
		sec_timer += delta;		// update sec_timer
		if(sec_timer > 1.0){ sec_timer -= 1.0; show_menu_text = !show_menu_text;}
		
		// GAME OVER		
		if(game_over){
			game_over_counter = MIN(game_over_counter + delta, 5); // clamping counter
			SetLayerTint(game_layer,olc::WHITE);
			if(score == 0) score += level_score;
			short r,g,b;
			HSVtoRGB(mapRange(0,1,1,359,sec_timer),100,100,r,g,b);
			DrawString(45,50,"GAME OVER",olc::WHITE,6);
			DrawString(130,170,"Your score:", olc::WHITE, 3);
						
			const int scale = (int)mapRange(0,score, 3,6,fancy_game_over_counter);
			
			fancy_game_over_counter = lerp(0,score,EaseOut(game_over_counter / 5));
			DrawString(130,240,std::to_string(fancy_game_over_counter),olc::Pixel(r,g,b), scale);
			DrawString(160,500,"Open game again to restart",olc::Pixel(255,255,255,50),1);
			if(GetKey(olc::TAB).bHeld)	DrawString(160,470,"Yeah im lazy how did you know",olc::Pixel(255,255,255,50),1);
		}
		
		// GAMEPLAY
		else {
			if(isLevelFinished(levels_data[curr_level])){
				if(curr_level == 0) DrawString(220,200,":)",olc::DARK_CYAN,6);
				else{
					in_transition = true;
					ball.speed = 0;
					player.can_control = false;
				}
			}
			
			if(in_transition){
				if(fade_or_move) Transition(game_layer,255,curr_time_anim); // fade from black
				else			 Transition(game_layer,1,curr_time_anim);   // move up 
				curr_time_anim += delta;
				sad_face_draw_easter_egg_never_find_free_HD = false;
				if(curr_time_anim > ANIM_DUR){ // end of animation
					if(!fade_or_move){ // finished moving up, now load the level while it fades in
						fade_or_move = true;
						SetLayerTint(game_layer,olc::BLACK);
						SetLayerOffset(game_layer,0,0);
						//reseting player and ball
						player.shooting_ball = true;
						player.pos = {200,450};
						player.lives = 3;
						player.powerup_timer = 0;
						ball.powerup_timer = 0;
						ball.speed = 250;
						// getting next level
						if(curr_level < LEVEL_COUNT-1){ curr_level++; DownloadLevel(levels_data[curr_level],&level_layout[curr_level]);}
						else game_over = true;
					}
					else{
						in_transition = false;
						fade_or_move = false;
						SetLayerTint(game_layer,olc::WHITE); // just to be sure heh
						player.can_control = true;
					}
					curr_time_anim = 0;
					if(curr_level != 1) //don't add up points after first level (its title level)
						score += level_score;
					level_score = 0;
				}
				
			}
			if(!game_over)DrawLevel(levels_data[curr_level]);
			DrawPad({player.pos.x,player.pos.y},player.length, olc::RED);
			FillCircle(ball.pos.x, ball.pos.y, ball.size ,ball.col);
			if(sad_face_draw_easter_egg_never_find_free_HD) DrawString(220,200,":(",olc::DARK_RED,6);
			
			if(player.shooting_ball){
				ball.pos = {player.pos.x + (player.length / 2),player.pos.y - 30};
				if(((GetKey(olc::SPACE).bReleased || GetKey(olc::UP).bReleased) || (sec_timer > 0.7 && player.auto_pilot)) && player.can_control ){ // after a second it releases the ball
					player.shooting_ball = false;
					ball.speed = 250;
					ball.dir.y = -0.20;
				}
			}
			
			if(in_menu)	DrawString(210,490,(show_menu_text ? "" : "Press Key..."),olc::WHITE,1);
			
			if(!in_menu && !in_transition) DrawUI(player);
			
			if(player.powerup_timer > 0) player.powerup_timer -= delta;
			else{
				player.length = 100; // some animation here? nah
				player.powerup_timer = 0;
			}
			
			if(ball.powerup_timer > 0) ball.powerup_timer -= delta;
			else{
				ball.col = olc::WHITE;
				if (ball.powerup_timer < 0) ball.speed = (ball.speed == 200 ? 200 : ball.saved_speed);
				ball.powerup_timer = 0;
			}
			// Logic
			if(player.auto_pilot && !player.shooting_ball){
				if(AnyInput()) {
					player.auto_pilot = false;
					in_menu = false;
					in_transition = true;
					ball.speed = 0;
				}
				player.pos.x = lerp(player.pos.x,MIN(MAX(ball.pos.x - player.rand_off_apilot, 6),WIDTH- player.length - 6), 0.009);
			}
			else{
				if(player.can_control){
					if(GetKey(olc::Key::LEFT).bHeld){
						if (player.shooting_ball)	ball.dir.x = -0.60;
						player.pos.x = MAX(player.pos.x - player.speed * delta, 6);
					}
					else if(GetKey(olc::Key::RIGHT).bHeld){
						if (player.shooting_ball)	ball.dir.x = 0.60;
						player.pos.x = MIN(player.pos.x + player.speed * delta, WIDTH- player.length - 6);
					}
				}
			}
			
			if(player.fail_transition){
				curr_time_anim += delta;
				
				// we use saved_fail because values all get fucked up and we want to animate pad and ball go from their first position to the default one so ye
				FailTransition(200 - saved_fail.player_x,saved_fail.player_x,  player.pos.x ,curr_time_anim); // setting x player
				FailTransition(100 - saved_fail.player_len,saved_fail.player_len,player.length,curr_time_anim); // setting length player
				
				FailTransition(250 - saved_fail.ball_x,saved_fail.ball_x,ball.pos.x,curr_time_anim); // setting x ball
				FailTransition(420 - saved_fail.ball_y,saved_fail.ball_y,ball.pos.y,curr_time_anim); // setting y ball
				
				if(curr_time_anim >= 2.0){
					player.fail_transition = false;
					curr_time_anim = 0;
					player.can_control = true;
					saved_fail.player_x = 0; // zero it so it can be overwritten in future
				}
			}
			if(!in_transition && !player.fail_transition) MoveBall(ball,levels_data[curr_level],player,delta);
			
			//DEV TOOLS
			if(GetKey(olc::PERIOD).bPressed) {
				in_transition = true;
			}
			if(GetKey(olc::COMMA).bPressed){
				in_transition = true;
				curr_level -= 2;
			}
			if(GetKey(olc::F3).bPressed){
				game_over = true;
			}

		}
		SetDrawTarget(nullptr);
		Clear(olc::BLANK);
		return true;
}
};

int main()
{
	Game demo;
	if (demo.Construct(WIDTH, HEIGHT, 2, 2))
		demo.Start();
	return 0;
}
