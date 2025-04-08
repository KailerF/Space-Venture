#include "raylib.h"
#include <string>
#include <vector>
#include <cmath>
#include <algorithm>    // Needed for std::remove_if
#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

//------------------ Game States & Global Variables ----------------------
enum GameState { MAIN_MENU, SETTINGS, CHARACTER_CREATION, CHARACTER_CUSTOMIZATION, PLAYING, PLATFORMER, LEVEL_COMPLETE, SPACESHIP_COMBAT };
GameState gameState = MAIN_MENU;

enum CustomizationTab { TAB_APPEARANCE, TAB_ATTRIBUTES, TAB_EQUIPMENT };
CustomizationTab currentTab = TAB_APPEARANCE;

// Physics and controls
const float GRAVITY = 0.5f;
const float JUMP_FORCE = -12.0f;
const float MOVE_SPEED = 5.0f;

std::string playerName = "";
char nameInput[20] = "";
int nameIndex = 0;

// Character appearance options
int selectedHairstyle = 0;
int selectedHairColor = 0;
int selectedSkinColor = 0;
int selectedEyeColor = 0;
int selectedFaceStyle = 0;
int selectedPlayerAppearance = 0; // Track selected player appearance/spacesuit
int selectedBeardStyle = 0; // Added beard style
bool hasHelmet = false; // Toggle for helmet (off in customization, on in gameplay)

// Character attributes
int selectedFightingClass = 0;
int strengthPoints = 5;
int agilityPoints = 5;
int intelligencePoints = 5;
int totalAttributePoints = 5; // Additional points

// Character equipment
int selectedWeapon = 0;
int selectedArmor = 0;
int selectedAccessory = 0;

// Player stats for gameplay
int playerHealth = 100;
int playerMaxHealth = 100;
int playerEnergy = 100;
int playerMaxEnergy = 100;
int playerScore = 0;
int playerCurrency = 0;  // Currency that player earns

// Global pause flag for level
bool isPaused = false;

//------------------ Platformer Structures ----------------------
struct PlayerData {
    Rectangle rect;
    Vector2 velocity;
    bool isJumping;
    bool canJump;
    bool facingRight;
    int health;
    int score;
    int currency;  // Player's currency
    Color skinColor;
    Color hairColor;
    int hairstyle;
    int appearance; // Store the selected appearance/spacesuit
    int beardStyle; // Store beard style
    int energy;
};

PlayerData player;

struct Enemy {
    Rectangle rect;
    Vector2 velocity;
    bool active;
    bool facingRight;
    int health;
    int type; // 0: Basic, 1: Flying, 2: Heavy
    float timer; // For behavior timing
    int currencyValue; // How much currency this enemy is worth
    Color primaryColor; // For programmatic enemy drawing
    Color secondaryColor; // For programmatic enemy drawing
};

std::vector<Enemy> enemies;

struct Platform {
    Rectangle rect;
    bool deadly; // Spikes/hazards
    int type; // 0: Normal, 1: Moving, 2: Breakable
    Vector2 velocity; // For moving platforms
};

std::vector<Platform> platforms;

struct Projectile {
    Rectangle rect;
    Vector2 velocity;
    bool active;
    bool fromPlayer;
    int damage;
};

std::vector<Projectile> projectiles;

struct LevelPortal {
    Rectangle rect;
    bool active;
    int targetLevel;
};

LevelPortal levelExit;

// Level system variables
Rectangle levelBounds = { 0, 0, 4000, 720 };
Vector2 cameraOffset = { 0, 0 };
int currentLevel = 1;
int maxLevel = 3;  // Total number of levels
bool levelCompleted = false;
int levelCompletionBonus = 500; // Currency bonus for completing a level

// Customization option texts
const char *hairstyles[] = {"Short", "Medium", "Long", "Mohawk", "Bald"};
const char *hairColors[] = {"Black", "Brown", "Blonde", "Red", "White"};
const char *skinColors[] = {"Light", "Tan", "Dark"};
const char *eyeColors[] = {"Blue", "Green", "Brown", "Gray"};
const char *faceStyles[] = {"Round", "Square", "Oval"};
const char *beardStyles[] = {"None", "Stubble", "Full", "Goatee"};
const char *fightingClasses[] = {"Expert Pilot", "Soldier", "Hacker"};
const char *weapons[] = {"Blaster Pistol", "Plasma Rifle", "Neural Disruptor"};
const char *armors[] = {"Stealth Suit", "Combat Armor", "Power Exoskeleton"};
const char *accessories[] = {"Wrist Computer", "Neural Implant", "Holographic Badge"};
const char *playerAppearanceNames[] = {"Standard Spacesuit", "Tactical Spacesuit", "Elite Spacesuit"}; // Player appearance names

//------------------ Settings ----------------------
int selectedResolution = 0;
const char *resolutions[] = {"1280x720", "1920x1080", "2560x1440"};
int screenWidth = 1280, screenHeight = 720;

//------------------ Visual Customization ----------------------
// Spacesuit Colors
Color suitColors[3] = {
    (Color){100, 100, 200, 255}, // Standard - Blue
    (Color){80, 120, 80, 255},   // Tactical - Green
    (Color){200, 100, 100, 255}  // Elite - Red
};

// Helmet colors (slightly darker than suit)
Color helmetColors[3] = {
    (Color){70, 70, 170, 255},   // Standard - Dark Blue
    (Color){60, 100, 60, 255},   // Tactical - Dark Green
    (Color){170, 70, 70, 255}    // Elite - Dark Red
};

// Enemy Colors
Color enemyPrimaryColors[3] = {
    (Color){180, 50, 50, 255},    // Basic - Red
    (Color){50, 50, 180, 255},    // Flying - Blue
    (Color){120, 40, 120, 255}    // Heavy - Purple
};

Color enemySecondaryColors[3] = {
    (Color){120, 30, 30, 255},    // Basic - Dark Red
    (Color){30, 30, 120, 255},    // Flying - Dark Blue
    (Color){80, 20, 80, 255}      // Heavy - Dark Purple
};

//------------------ Audio ----------------------
Font customFont;
Music backgroundMusic;
Sound jumpSound;
Sound shootSound;
Sound hitSound;
Sound laserSound; // For spaceship lasers
Sound coinSound;
Sound portalSound;
Sound levelCompleteSound;

float musicVolume = 0.5f;
bool isMusicPaused = false;

//------------------ Collectibles ----------------------
struct Collectible {
    Rectangle rect;
    bool active;
    int value;
    int type; // 0: Coin, 1: Health, 2: Powerup
};

std::vector<Collectible> collectibles;

//------------------ Utility Functions ----------------------
float GetScaleFactor() {
    return (float)GetScreenWidth() / 1280.0f;
}

//------------------ Function Declarations ----------------------
void DrawMainMenu();
void DrawSettingsMenu();
void DrawCharacterCreation();
void DrawCharacterCustomization();
void DrawPlaying();
void DrawPlatformer();
void DrawLevelComplete();
void DrawSpaceCombat();
void DrawDetailedCharacter(float x, float y, float scale, bool withHelmet);
void DrawDetailedSpace(float offsetX);
void DrawDetailedEnemy(Enemy &enemy);
void DrawSpikes(float x, float y, float width, float height);
void InitPlatformerLevel(int level);
void UpdatePlatformer();
void SpawnEnemy(float x, float y, int type);
void SpawnCollectible(float x, float y, int type);
void ShootProjectile(float x, float y, float velX, bool fromPlayer, int damage);
bool CheckCollisionWithPlatforms(Rectangle rect);
void TransitionToGameplay();
void TransitionToNextLevel();
void CreateLevelLayout(int level);

void ToggleMusicPause();
void SetMusicVolume(float volume);

//------------------ Music Control Functions ----------------------
void ToggleMusicPause() {
    if (isMusicPaused) PlayMusicStream(backgroundMusic);
    else PauseMusicStream(backgroundMusic);
    isMusicPaused = !isMusicPaused;
}

void SetMusicVolume(float volume) {
    musicVolume = volume;
    SetMusicVolume(backgroundMusic, musicVolume);
}

//------------------ Level Management ----------------------
void CreateLevelLayout(int level) {
    platforms.clear();
    enemies.clear();
    projectiles.clear();
    collectibles.clear();
    
    // Common ground platforms
    for (int i = 0; i < 40; i++) {
        Platform plat;
        plat.rect = (Rectangle){ i * 100.0f, 650.0f, 100.0f, 30.0f };
        plat.deadly = false;
        plat.type = 0;
        plat.velocity = (Vector2){ 0, 0 };
        platforms.push_back(plat);
    }

    // Different level layouts
    if (level == 1) {
        // Level 1: Beginner level with simple platforms and few enemies
        platforms.push_back({ {300, 500, 200, 30}, false, 0, {0,0} });
        platforms.push_back({ {600, 400, 150, 30}, false, 0, {0,0} });
        platforms.push_back({ {900, 350, 200, 30}, false, 0, {0,0} });
        
        // Floating platforms for jumping challenge
        platforms.push_back({ {400, 300, 80, 20}, false, 0, {0,0} });
        platforms.push_back({ {520, 250, 60, 20}, false, 0, {0,0} });
        platforms.push_back({ {650, 220, 50, 20}, false, 0, {0,0} });
        
        // Add some hazards (spikes)
        platforms.push_back({ {800, 630, 100, 20}, true, 0, {0,0} });
        
        // Basic enemies
        SpawnEnemy(500, 600, 0);
        SpawnEnemy(950, 300, 0);
        
        // Coins
        SpawnCollectible(350, 450, 0);
        SpawnCollectible(650, 350, 0);
        SpawnCollectible(950, 300, 0);
        
        // Add coins on the jumping challenge path
        SpawnCollectible(400, 270, 0);
        SpawnCollectible(520, 220, 0);
        SpawnCollectible(650, 190, 0);
        
        // Set level exit
        levelExit.rect = (Rectangle){ 1200, 550, 60, 100 };
        levelExit.active = true;
        levelExit.targetLevel = 2;
        
    } else if (level == 2) {
        // Level 2: More complex with moving platforms and more enemies
        platforms.push_back({ {300, 500, 200, 30}, false, 0, {0,0} });
        platforms.push_back({ {600, 400, 150, 30}, false, 0, {0,0} });
        platforms.push_back({ {900, 350, 200, 30}, false, 0, {0,0} });
        platforms.push_back({ {1300, 450, 150, 30}, false, 1, {1.0f, 0} });
        platforms.push_back({ {1600, 550, 120, 30}, false, 2, {0,0} });
        platforms.push_back({ {1900, 500, 120, 30}, false, 2, {0,0} });
        
        // Additional platforms for traversal
        platforms.push_back({ {1100, 300, 80, 20}, false, 0, {0,0} });
        platforms.push_back({ {1200, 250, 80, 20}, false, 0, {0,0} });
        platforms.push_back({ {1350, 200, 60, 20}, false, 1, {0, 1.5f} }); // Vertically moving platform
        
        // Hazards
        platforms.push_back({ {800, 630, 100, 20}, true, 0, {0,0} });
        platforms.push_back({ {1400, 630, 100, 20}, true, 0, {0,0} });
        platforms.push_back({ {1700, 630, 100, 20}, true, 0, {0,0} });
        
        // Mix of enemies
        SpawnEnemy(500, 600, 0);
        SpawnEnemy(700, 350, 1);  // Flying enemy
        SpawnEnemy(950, 300, 0);
        SpawnEnemy(1500, 400, 0);
        SpawnEnemy(1800, 450, 1);  // Flying enemy
        
        // More coins
        SpawnCollectible(350, 450, 0);
        SpawnCollectible(650, 350, 0);
        SpawnCollectible(950, 300, 0);
        SpawnCollectible(1350, 400, 0);
        SpawnCollectible(1700, 500, 0);
        SpawnCollectible(1950, 450, 0);
        
        // Coins along the challenging path
        SpawnCollectible(1100, 270, 0);
        SpawnCollectible(1200, 220, 0);
        SpawnCollectible(1350, 170, 0);
        
        // Health pickup
        SpawnCollectible(1200, 600, 1);
        
        // Level exit
        levelExit.rect = (Rectangle){ 2200, 550, 60, 100 };
        levelExit.active = true;
        levelExit.targetLevel = 3;
        
    } else if (level == 3) {
        // Level 3: Challenging with more hazards, heavy enemies, and complex platform arrangement
        platforms.push_back({ {300, 500, 200, 30}, false, 0, {0,0} });
        platforms.push_back({ {600, 400, 150, 30}, false, 0, {0,0} });
        platforms.push_back({ {900, 350, 200, 30}, false, 0, {0,0} });
        platforms.push_back({ {1300, 450, 150, 30}, false, 1, {1.0f, 0} });
        platforms.push_back({ {1600, 550, 120, 30}, false, 2, {0,0} });
        platforms.push_back({ {1900, 500, 120, 30}, false, 2, {0,0} });
        platforms.push_back({ {2200, 600, 100, 30}, false, 0, {0,0} });
        platforms.push_back({ {2500, 550, 150, 30}, false, 1, {1.2f, 0} });
        platforms.push_back({ {2800, 450, 200, 30}, false, 0, {0,0} });
        platforms.push_back({ {3200, 400, 150, 30}, false, 1, {1.5f, 0} });
        
        // Complex platform arrangements
        // Stairway up
        platforms.push_back({ {2900, 350, 60, 20}, false, 0, {0,0} });
        platforms.push_back({ {3000, 300, 60, 20}, false, 0, {0,0} });
        platforms.push_back({ {3100, 250, 60, 20}, false, 0, {0,0} });
        
        // Moving platform challenges
        platforms.push_back({ {2600, 300, 80, 20}, false, 1, {0, 2.0f} }); // Vertical mover
        platforms.push_back({ {2800, 250, 60, 20}, false, 1, {1.8f, 0} }); // Horizontal mover
        
        // Breakable platform sequence
        platforms.push_back({ {1750, 450, 60, 20}, false, 2, {0,0} });
        platforms.push_back({ {1850, 400, 60, 20}, false, 2, {0,0} });
        platforms.push_back({ {1950, 350, 60, 20}, false, 2, {0,0} });
        
        // More hazards
        platforms.push_back({ {800, 630, 100, 20}, true, 0, {0,0} });
        platforms.push_back({ {1400, 630, 100, 20}, true, 0, {0,0} });
        platforms.push_back({ {2000, 630, 100, 20}, true, 0, {0,0} });
        platforms.push_back({ {2600, 630, 100, 20}, true, 0, {0,0} });
        platforms.push_back({ {3000, 630, 100, 20}, true, 0, {0,0} });
        
        // Advanced enemy placement
        SpawnEnemy(500, 600, 0);
        SpawnEnemy(700, 350, 1);  // Flying enemy
        SpawnEnemy(1100, 600, 2); // Heavy enemy
        SpawnEnemy(1500, 400, 0);
        SpawnEnemy(1900, 450, 1); // Flying enemy
        SpawnEnemy(2400, 500, 2); // Heavy enemy
        SpawnEnemy(2900, 400, 1); // Flying enemy
        SpawnEnemy(3300, 350, 2); // Heavy enemy
        
        // Lots of coins
        for (int i = 0; i < 20; i++) {
            float x = GetRandomValue(300, 3500);
            float y = GetRandomValue(200, 500);
            SpawnCollectible(x, y, 0);
        }
        
        // Coins along challenge paths
        SpawnCollectible(2900, 320, 0);
        SpawnCollectible(3000, 270, 0);
        SpawnCollectible(3100, 220, 0);
        
        SpawnCollectible(2600, 270, 0);
        SpawnCollectible(2800, 220, 0);
        
        SpawnCollectible(1750, 420, 0);
        SpawnCollectible(1850, 370, 0);
        SpawnCollectible(1950, 320, 0);
        
        // Health pickups
        SpawnCollectible(1200, 600, 1);
        SpawnCollectible(2300, 550, 1);
        
        // Power-ups
        SpawnCollectible(1700, 500, 2);
        SpawnCollectible(3000, 400, 2);
        
        // Level exit - This is the final level
        levelExit.rect = (Rectangle){ 3500, 550, 60, 100 };
        levelExit.active = true;
        levelExit.targetLevel = 1; // Loop back to level 1 after completing level 3
    }
    
    // Update level boundaries based on level
    if (level == 1) {
        levelBounds.width = 1500;
    } else if (level == 2) {
        levelBounds.width = 2500;
    } else if (level == 3) {
        levelBounds.width = 4000;
    }
    
    cameraOffset = (Vector2){ 0, 0 };
}

void InitPlatformerLevel(int level) {
    // Initialize player
    player.rect = (Rectangle){ 100, 300, 80, 120 }; // Increased player size
    player.velocity = (Vector2){ 0, 0 };
    player.isJumping = false;
    player.canJump = false;
    player.facingRight = true;
    
    // Don't reset player health between levels unless they died
    if (gameState != LEVEL_COMPLETE) {
        player.health = playerHealth;
        player.score = 0;
        player.currency = 0;
    }
    
    // Set player appearance based on customization
    player.appearance = selectedPlayerAppearance;
    player.beardStyle = selectedBeardStyle;
    player.hairstyle = selectedHairstyle;
    
    // Set colors based on customization
    switch(selectedSkinColor) {
        case 0: player.skinColor = (Color){255,220,177,255}; break;
        case 1: player.skinColor = (Color){240,184,130,255}; break;
        case 2: player.skinColor = (Color){165,114,90,255}; break;
        default: player.skinColor = (Color){255,220,177,255};
    }
    
    switch(selectedHairColor) {
        case 0: player.hairColor = (Color){30,30,30,255}; break;
        case 1: player.hairColor = (Color){139,69,19,255}; break;
        case 2: player.hairColor = (Color){255,215,0,255}; break;
        case 3: player.hairColor = (Color){178,34,34,255}; break;
        case 4: player.hairColor = (Color){220,220,220,255}; break;
        default: player.hairColor = (Color){30,30,30,255};
    }
    
    // Enable helmet in gameplay
    hasHelmet = true;
    
    // Create the level layout
    CreateLevelLayout(level);
    
    currentLevel = level;
    levelCompleted = false;
}
void TransitionToGameplay() {
    gameState = PLATFORMER;
    InitPlatformerLevel(1); // Start with level 1
}

void TransitionToNextLevel() {
    // Award completion bonus
    player.currency += levelCompletionBonus;
    playerCurrency = player.currency;
    
    if (levelExit.targetLevel <= maxLevel) {
        gameState = LEVEL_COMPLETE;
        levelCompleted = true;
    } else {
        // If this was the final level, go back to main menu
        gameState = MAIN_MENU;
    }
}

//------------------ Spawning Functions ----------------------
void SpawnEnemy(float x, float y, int type) {
    Enemy enemy;
    enemy.active = true;
    enemy.facingRight = GetRandomValue(0, 1) == 1;
    enemy.timer = 0;
    
    // Set primary and secondary colors for the enemy based on type
    enemy.primaryColor = enemyPrimaryColors[type];
    enemy.secondaryColor = enemySecondaryColors[type];
    
    if (type == 0) {
        enemy.rect = (Rectangle){ x, y, 60, 80 }; // Basic enemy - bigger size
        enemy.velocity = (Vector2){ enemy.facingRight ? 2.0f : -2.0f, 0 };
        enemy.health = 3;
        enemy.currencyValue = 10;
    } else if (type == 1) {
        enemy.rect = (Rectangle){ x, y, 70, 60 }; // Flying enemy - bigger size
        enemy.velocity = (Vector2){ enemy.facingRight ? 3.0f : -3.0f, 0 };
        enemy.health = 2;
        enemy.currencyValue = 15;
    } else if (type == 2) {
        enemy.rect = (Rectangle){ x, y, 80, 100 }; // Heavy enemy - bigger size
        enemy.velocity = (Vector2){ enemy.facingRight ? 1.0f : -1.0f, 0 };
        enemy.health = 5;
        enemy.currencyValue = 25;
    }
    
    enemy.type = type;
    enemies.push_back(enemy);
}

void SpawnCollectible(float x, float y, int type) {
    Collectible collectible;
    collectible.active = true;
    
    if (type == 0) { // Coin
        collectible.rect = (Rectangle){ x, y, 30, 30 };
        collectible.value = 5;
    } else if (type == 1) { // Health
        collectible.rect = (Rectangle){ x, y, 40, 40 };
        collectible.value = 20;
    } else if (type == 2) { // Powerup
        collectible.rect = (Rectangle){ x, y, 40, 40 };
        collectible.value = 10;
    }
    
    collectible.type = type;
    collectibles.push_back(collectible);
}

//------------------ Gameplay Functions ----------------------
void ShootProjectile(float x, float y, float velX, bool fromPlayer, int damage) {
    Projectile proj;
    proj.rect = (Rectangle){ x, y, 15, 8 };
    proj.velocity = (Vector2){ velX, 0 };
    proj.active = true;
    proj.fromPlayer = fromPlayer;
    proj.damage = damage;
    projectiles.push_back(proj);
    PlaySound(shootSound);
}

bool CheckCollisionWithPlatforms(Rectangle rect) {
    for (auto& platform : platforms) {
        if (CheckCollisionRecs(rect, platform.rect)) return true;
    }
    return false;
}

//------------------ Drawing Functions ----------------------
void DrawDetailedSpace(float offsetX) {
    // Draw space background
    ClearBackground((Color){10, 5, 30, 255}); // Deep space color
    
    // Draw distant stars (small white dots)
    for (int i = 0; i < 200; i++) {
        float x = (float)((i * 37) % (int)GetScreenWidth()) - offsetX * 0.1f;
        if (x < 0) x += GetScreenWidth();
        if (x > GetScreenWidth()) x -= GetScreenWidth();
        
        float y = (float)((i * 53) % (int)GetScreenHeight());
        float size = (i % 3) + 1; // Random star size (1-3)
        
        // Add twinkle effect
        float brightness = 0.7f + 0.3f * sinf(GetTime() * (0.5f + i * 0.01f));
        
        DrawCircle(x, y, size, (Color){
            (unsigned char)(255 * brightness), 
            (unsigned char)(255 * brightness), 
            (unsigned char)(255 * brightness), 
            255
        });
    }
    
    // Draw a colorful nebula
    for (int i = 0; i < 5; i++) {
        float x = (float)((i * 233 + 120) % (int)GetScreenWidth() * 2) - offsetX * 0.2f;
        if (x < -300) x += GetScreenWidth() * 2;
        if (x > GetScreenWidth() + 300) x -= GetScreenWidth() * 2;
        
        float y = (float)((i * 157 + 50) % (int)GetScreenHeight());
        float radius = 100.0f + i * 30.0f;
        
        // Create nebula colors
        Color nebulaColor;
        switch (i % 5) {
            case 0: nebulaColor = (Color){80, 40, 120, 40}; break; // Purple
            case 1: nebulaColor = (Color){120, 40, 80, 40}; break; // Pink
            case 2: nebulaColor = (Color){40, 80, 120, 40}; break; // Blue
            case 3: nebulaColor = (Color){120, 80, 40, 40}; break; // Orange
            case 4: nebulaColor = (Color){40, 120, 80, 40}; break; // Green
        }
        
        DrawCircleGradient(x, y, radius, nebulaColor, BLANK);
    }
    
    // Draw a large distant planet
    float planetX = GetScreenWidth() * 0.8f - offsetX * 0.15f;
    if (planetX < -200) planetX += GetScreenWidth() * 1.5f;
    if (planetX > GetScreenWidth() + 200) planetX -= GetScreenWidth() * 1.5f;
    
    float planetY = GetScreenHeight() * 0.3f;
    float planetRadius = 150.0f;
    
    // Planet body
    DrawCircleGradient(planetX, planetY, planetRadius, 
                      (Color){80, 40, 100, 255}, 
                      (Color){50, 20, 70, 255});
    
    // Planet surface details
    for (int i = 0; i < 10; i++) {
        float angle = i * 0.628f; // Spread around the planet
        float distance = 0.7f * planetRadius;
        float detailX = planetX + cosf(angle) * distance;
        float detailY = planetY + sinf(angle) * distance;
        float detailSize = (i % 3) * 10.0f + 5.0f;
        
        DrawCircleGradient(detailX, detailY, detailSize, 
                          (Color){100, 50, 120, 100}, 
                          (Color){70, 30, 90, 0});
    }
    
    // Draw a ring around the planet
    float innerRadius = planetRadius * 1.2f;
    float outerRadius = planetRadius * 1.6f;
    for (float r = innerRadius; r <= outerRadius; r += 0.5f) {
        // Vary opacity to create ring appearance
        unsigned char alpha = (unsigned char)(100 - (r - innerRadius) / (outerRadius - innerRadius) * 80);
        DrawCircleLines(planetX, planetY, r, (Color){150, 120, 180, alpha});
    }
    
    // Draw smaller moons
    for (int i = 0; i < 2; i++) {
        float angle = GetTime() * 0.2f + i * 3.14f; // Rotation around planet
        float distance = planetRadius * (1.8f + i * 0.3f);
        float moonX = planetX + cosf(angle) * distance;
        float moonY = planetY + sinf(angle) * distance;
        float moonRadius = planetRadius * (0.15f + i * 0.05f);
        
        // Draw moon
        DrawCircleGradient(moonX, moonY, moonRadius, 
                          (Color){200, 200, 200, 255}, 
                          (Color){120, 120, 120, 255});
        
        // Draw moon craters
        for (int j = 0; j < 3; j++) {
            float craterAngle = j * 2.1f;
            float craterDistance = moonRadius * 0.5f;
            float craterX = moonX + cosf(craterAngle) * craterDistance;
            float craterY = moonY + sinf(craterAngle) * craterDistance;
            float craterRadius = moonRadius * 0.2f;
            
            DrawCircleGradient(craterX, craterY, craterRadius,
                              (Color){100, 100, 100, 150},
                              (Color){80, 80, 80, 50});
        }
    }
}

void DrawSpikes(float x, float y, float width, float height) {
    // Draw spikes as triangles
    int numSpikes = (int)(width / 10.0f);
    float spikeWidth = width / numSpikes;
    
    for (int i = 0; i < numSpikes; i++) {
        float spikeX = x + i * spikeWidth;
        
        // Draw triangle for each spike
        DrawTriangle(
            (Vector2){spikeX, y + height},
            (Vector2){spikeX + spikeWidth * 0.5f, y},
            (Vector2){spikeX + spikeWidth, y + height},
            (Color){150, 150, 150, 255}
        );
        
        // Draw metallic highlight
        DrawLineEx(
            (Vector2){spikeX + spikeWidth * 0.25f, y + height * 0.5f},
            (Vector2){spikeX + spikeWidth * 0.5f, y + height * 0.1f},
            2.0f,
            (Color){220, 220, 220, 200}
        );
    }
    
    // Draw base
    DrawRectangle(x, y + height - 5, width, 5, (Color){100, 100, 100, 255});
}

void DrawDetailedEnemy(Enemy &enemy) {
    float x = enemy.rect.x;
    float y = enemy.rect.y;
    float width = enemy.rect.width;
    float height = enemy.rect.height;
    bool facingRight = enemy.facingRight;
    
    // Draw based on enemy type
    switch(enemy.type) {
        case 0: // Basic enemy - Alien Soldier
        {
            // Body
            DrawRectangleRounded(
                (Rectangle){x + width * 0.2f, y + height * 0.3f, width * 0.6f, height * 0.5f},
                0.3f, 10, enemy.primaryColor
            );
            
            // Head
            DrawCircle(
                x + (facingRight ? (width * 0.6f) : (width * 0.4f)),
                y + height * 0.2f,
                width * 0.2f,
                enemy.primaryColor
            );
            
            // Eyes (with glow effect)
            float eyeX = x + (facingRight ? (width * 0.7f) : (width * 0.3f));
            DrawCircle(eyeX, y + height * 0.18f, width * 0.08f, (Color){220, 220, 50, 255});  // Yellow glow
            DrawCircle(eyeX, y + height * 0.18f, width * 0.05f, (Color){255, 255, 150, 255}); // Brighter center
            
            // Arms
            DrawRectangleRounded(
                (Rectangle){x + (facingRight ? width * 0.7f : width * 0.1f), y + height * 0.35f, width * 0.2f, height * 0.3f},
                0.5f, 10, enemy.secondaryColor
            );
            
            // Legs
            DrawRectangleRounded(
                (Rectangle){x + width * 0.25f, y + height * 0.75f, width * 0.2f, height * 0.25f},
                0.3f, 10, enemy.secondaryColor
            );
            DrawRectangleRounded(
                (Rectangle){x + width * 0.55f, y + height * 0.75f, width * 0.2f, height * 0.25f},
                0.3f, 10, enemy.secondaryColor
            );
            
            // Weapon
            float weaponX = x + (facingRight ? width * 0.9f : 0);
            DrawRectangle(
                weaponX, y + height * 0.4f,
                facingRight ? width * 0.2f : -width * 0.2f,
                height * 0.1f,
                (Color){50, 50, 50, 255}
            );
            
            // Armor details
            DrawRectangleRounded(
                (Rectangle){x + width * 0.3f, y + height * 0.3f, width * 0.4f, height * 0.1f},
                0.5f, 8, enemy.secondaryColor
            );
            
            // Helmet visor
            DrawRectangleRounded(
                (Rectangle){x + (facingRight ? width * 0.55f : width * 0.25f), y + height * 0.13f, width * 0.2f, height * 0.07f},
                0.5f, 8, (Color){150, 220, 255, 180}
            );
            break;
        }
        
        case 1: // Flying enemy - Alien Drone
        {
            // Body - UFO-shaped
            DrawCircle(
                x + width * 0.5f,
                y + height * 0.4f,
                width * 0.4f,
                enemy.primaryColor
            );
            
            // Top dome
            DrawCircle(
                x + width * 0.5f,
                y + height * 0.3f,
                width * 0.25f,
                enemy.secondaryColor
            );
            
            // Bottom section
            DrawRectangleRounded(
                (Rectangle){x + width * 0.3f, y + height * 0.4f, width * 0.4f, height * 0.1f},
                0.5f, 8, enemy.secondaryColor
            );
            
            // Thruster flames (pulsing)
            float pulseSize = 0.1f + 0.05f * sinf(GetTime() * 10);
            DrawCircle(
                x + width * 0.3f,
                y + height * 0.6f,
                width * pulseSize,
                (Color){255, 150, 50, 200}
            );
            DrawCircle(
                x + width * 0.5f,
                y + height * 0.6f,
                width * pulseSize,
                (Color){255, 150, 50, 200}
            );
            DrawCircle(
                x + width * 0.7f,
                y + height * 0.6f,
                width * pulseSize,
                (Color){255, 150, 50, 200}
            );
            
            // Lights (blinking)
            Color lightColor = {255, 255, 255, (unsigned char)(180 + 75 * sinf(GetTime() * 3))};
            DrawCircle(x + width * 0.2f, y + height * 0.4f, width * 0.05f, lightColor);
            DrawCircle(x + width * 0.5f, y + height * 0.5f, width * 0.05f, lightColor);
            DrawCircle(x + width * 0.8f, y + height * 0.4f, width * 0.05f, lightColor);
            
            // Eye/scanner - glowing orb
            float eyeX = x + (facingRight ? (width * 0.7f) : (width * 0.3f));
            DrawCircleGradient(
                eyeX, y + height * 0.3f,
                width * 0.15f,
                (Color){100, 200, 255, 255},
                (Color){220, 240, 255, 255}
            );
            break;
        }
        
        case 2: // Heavy enemy - Alien Brute
        {
            // Body - bulky and armored
            DrawRectangleRounded(
                (Rectangle){x + width * 0.15f, y + height * 0.3f, width * 0.7f, height * 0.5f},
                0.2f, 10, enemy.primaryColor
            );
            
            // Head - larger and intimidating
            DrawCircle(
                x + (facingRight ? (width * 0.65f) : (width * 0.35f)),
                y + height * 0.2f,
                width * 0.25f,
                enemy.primaryColor
            );
            
            // Shoulder plates
            DrawRectangleRounded(
                (Rectangle){x + width * 0.05f, y + height * 0.25f, width * 0.3f, height * 0.1f},
                0.3f, 8, enemy.secondaryColor
            );
            DrawRectangleRounded(
                (Rectangle){x + width * 0.65f, y + height * 0.25f, width * 0.3f, height * 0.1f},
                0.3f, 8, enemy.secondaryColor
            );
            
            // Arms - massive
            DrawRectangleRounded(
                (Rectangle){x + (facingRight ? width * 0.75f : width * 0.05f), y + height * 0.3f, width * 0.2f, height * 0.4f},
                0.3f, 10, enemy.secondaryColor
            );
            
            // Legs - heavy and armored
            DrawRectangleRounded(
                (Rectangle){x + width * 0.2f, y + height * 0.75f, width * 0.25f, height * 0.25f},
                0.2f, 10, enemy.secondaryColor
            );
            DrawRectangleRounded(
                (Rectangle){x + width * 0.55f, y + height * 0.75f, width * 0.25f, height * 0.25f},
                0.2f, 10, enemy.secondaryColor
            );
            
            // Eyes (glowing red)
            float leftEyeX = x + (facingRight ? (width * 0.55f) : (width * 0.3f));
            float rightEyeX = x + (facingRight ? (width * 0.75f) : (width * 0.4f));
            DrawCircle(leftEyeX, y + height * 0.15f, width * 0.06f, (Color){255, 50, 50, 255});
            DrawCircle(rightEyeX, y + height * 0.15f, width * 0.06f, (Color){255, 50, 50, 255});
            
            // Armor plating details
            DrawRectangleRounded(
                (Rectangle){x + width * 0.25f, y + height * 0.35f, width * 0.5f, height * 0.1f},
                0.5f, 8, enemy.secondaryColor
            );
            
            // Heavy weapon
            float weaponX = x + (facingRight ? width * 0.95f : -width * 0.3f);
            DrawRectangle(
                weaponX, y + height * 0.4f,
                facingRight ? width * 0.3f : width * 0.3f,
                height * 0.15f,
                (Color){80, 80, 80, 255}
            );
            
            // Weapon details
            float barrelX = x + (facingRight ? width * 1.15f : -width * 0.2f);
            DrawCircle(barrelX, y + height * 0.475f, width * 0.08f, (Color){50, 50, 50, 255});
            break;
        }
    }
}

void DrawDetailedCharacter(float x, float y, float scale, bool withHelmet) {
    float headSize = 30.0f * scale;
    float bodyWidth = 40.0f * scale;
    float bodyHeight = 60.0f * scale;
    
    // Get the correct colors based on selections
    Color suitColor = suitColors[selectedPlayerAppearance];
    Color helmetColor = helmetColors[selectedPlayerAppearance];
    
    // Get skin color based on selection
    Color skinColor;
    switch(selectedSkinColor) {
        case 0: skinColor = (Color){255, 220, 177, 255}; break; // Light
        case 1: skinColor = (Color){240, 184, 130, 255}; break; // Tan
        case 2: skinColor = (Color){165, 114, 90, 255}; break;  // Dark
        default: skinColor = (Color){255, 220, 177, 255};
    }
    
    // Get hair color based on selection
    Color hairColor;
    switch(selectedHairColor) {
        case 0: hairColor = (Color){30, 30, 30, 255}; break;     // Black
        case 1: hairColor = (Color){139, 69, 19, 255}; break;    // Brown
        case 2: hairColor = (Color){255, 215, 0, 255}; break;    // Blonde
        case 3: hairColor = (Color){178, 34, 34, 255}; break;    // Red
        case 4: hairColor = (Color){220, 220, 220, 255}; break;  // White
        default: hairColor = (Color){30, 30, 30, 255};
    }
    
    // Determine center positions
    float headX = x;
    float headY = y - bodyHeight * 0.25f;
    
    // Draw legs
    DrawRectangleRounded(
        (Rectangle){ x - bodyWidth * 0.25f, y + bodyHeight * 0.5f, bodyWidth * 0.2f, bodyHeight * 0.5f },
        0.3f, 8, suitColor
    );
    DrawRectangleRounded(
        (Rectangle){ x + bodyWidth * 0.05f, y + bodyHeight * 0.5f, bodyWidth * 0.2f, bodyHeight * 0.5f },
        0.3f, 8, suitColor
    );
    
    // Draw boots
    DrawRectangleRounded(
        (Rectangle){ x - bodyWidth * 0.3f, y + bodyHeight * 0.9f, bodyWidth * 0.3f, bodyHeight * 0.1f },
        0.3f, 8, helmetColor
    );
    DrawRectangleRounded(
        (Rectangle){ x + bodyWidth * 0.0f, y + bodyHeight * 0.9f, bodyWidth * 0.3f, bodyHeight * 0.1f },
        0.3f, 8, helmetColor
    );
    
    // Draw body/torso with spacesuit
    DrawRectangleRounded(
        (Rectangle){ x - bodyWidth * 0.35f, y - bodyHeight * 0.2f, bodyWidth * 0.7f, bodyHeight * 0.7f },
        0.3f, 8, suitColor
    );
    
    // Draw arms
    DrawRectangleRounded(
        (Rectangle){ x - bodyWidth * 0.5f, y, bodyWidth * 0.15f, bodyHeight * 0.4f },
        0.3f, 8, suitColor
    );
    DrawRectangleRounded(
        (Rectangle){ x + bodyWidth * 0.35f, y, bodyWidth * 0.15f, bodyHeight * 0.4f },
        0.3f, 8, suitColor
    );
    
    // Draw gloves
    DrawRectangleRounded(
        (Rectangle){ x - bodyWidth * 0.55f, y + bodyHeight * 0.3f, bodyWidth * 0.25f, bodyHeight * 0.15f },
        0.3f, 8, helmetColor
    );
    DrawRectangleRounded(
        (Rectangle){ x + bodyWidth * 0.3f, y + bodyHeight * 0.3f, bodyWidth * 0.25f, bodyHeight * 0.15f },
        0.3f, 8, helmetColor
    );
    
    // Draw spacesuit details (chest plate, life support, etc.)
    // Central chest unit
    DrawRectangleRounded(
        (Rectangle){ x - bodyWidth * 0.15f, y - bodyHeight * 0.05f, bodyWidth * 0.3f, bodyHeight * 0.2f },
        0.3f, 8, helmetColor
    );
    
    // Life support indicators (small lights)
    DrawCircle(x - bodyWidth * 0.05f, y, bodyWidth * 0.03f, GREEN);
    DrawCircle(x + bodyWidth * 0.05f, y, bodyWidth * 0.03f, BLUE);
    
    // Suit straps/seams
    DrawLineEx(
        (Vector2){ x - bodyWidth * 0.2f, y - bodyHeight * 0.2f },
        (Vector2){ x - bodyWidth * 0.2f, y + bodyHeight * 0.3f },
        2.0f, helmetColor
    );
    DrawLineEx(
        (Vector2){ x + bodyWidth * 0.2f, y - bodyHeight * 0.2f },
        (Vector2){ x + bodyWidth * 0.2f, y + bodyHeight * 0.3f },
        2.0f, helmetColor
    );
    
    // Draw belt
    DrawRectangle(
        x - bodyWidth * 0.35f, y + bodyHeight * 0.3f,
        bodyWidth * 0.7f, bodyHeight * 0.05f,
        helmetColor
    );
    
    // Draw head/face
    if (withHelmet) {
        // Draw helmet
        DrawCircle(headX, headY, headSize, helmetColor);
        
        // Draw visor (transparent front of helmet)
        DrawRectangleRounded(
            (Rectangle){ headX - headSize * 0.7f, headY - headSize * 0.4f, headSize * 1.4f, headSize * 0.8f },
            0.8f, 8, (Color){150, 220, 255, 180}
        );
        
        // Draw helmet details
        DrawLineEx(
            (Vector2){ headX - headSize * 0.5f, headY - headSize * 0.6f },
            (Vector2){ headX + headSize * 0.5f, headY - headSize * 0.6f },
            2.0f, suitColor
        );
        
        // Draw antenna
        DrawLineEx(
            (Vector2){ headX + headSize * 0.3f, headY - headSize * 0.8f },
            (Vector2){ headX + headSize * 0.3f, headY - headSize * 1.3f },
            2.0f, suitColor
        );
        DrawCircle(headX + headSize * 0.3f, headY - headSize * 1.3f, headSize * 0.1f, RED);
        
    } else {
        // Draw face (in customization view)
        DrawCircle(headX, headY, headSize, skinColor);
        
        // Draw eyes
        float eyeSpacing = headSize * 0.4f;
        Color eyeColor;
        switch(selectedEyeColor) {
            case 0: eyeColor = BLUE; break;
            case 1: eyeColor = GREEN; break;
            case 2: eyeColor = BROWN; break;
            case 3: eyeColor = GRAY; break;
            default: eyeColor = BLUE;
        }
        
        DrawCircle(headX - eyeSpacing * 0.5f, headY - headSize * 0.1f, headSize * 0.15f, WHITE);
        DrawCircle(headX + eyeSpacing * 0.5f, headY - headSize * 0.1f, headSize * 0.15f, WHITE);
        DrawCircle(headX - eyeSpacing * 0.5f, headY - headSize * 0.1f, headSize * 0.08f, eyeColor);
        DrawCircle(headX + eyeSpacing * 0.5f, headY - headSize * 0.1f, headSize * 0.08f, eyeColor);
        
        // Draw mouth
        DrawRectangleRounded(
            (Rectangle){ headX - headSize * 0.3f, headY + headSize * 0.3f, headSize * 0.6f, headSize * 0.1f },
            0.5f, 8, (Color){150, 80, 80, 255}
        );
        
        // Draw hair based on hairstyle
       // Draw hair based on hairstyle
// Find this section in DrawDetailedCharacter function that handles hair styles
// and replace it with this adjusted code:

// Draw hair based on hairstyle
// Find this section in DrawDetailedCharacter function that handles hair styles
// and replace it with this adjusted code:

// Draw hair based on hairstyle

switch(selectedHairstyle) {
    case 0: // Short hair
        DrawRectangleRounded(
            (Rectangle){ 
                headX - headSize * 1.0f,  // Wider coverage
                headY - headSize * 1.0f,  // Higher positioning
                headSize * 2.0f,  // Much wider
                headSize * 0.4f  // Thicker hair
            },
            0.3f, 8, hairColor
        );
        break;
        
    case 1: // Medium hair
        // Top hair band
        DrawRectangleRounded(
            (Rectangle){ 
                headX - headSize * 1.0f, 
                headY - headSize * 1.0f, 
                headSize * 2.0f, 
                headSize * 0.4f  // Thicker hair
            },
            0.3f, 8, hairColor
        );
        
        // Side hair - more proportional
        DrawRectangleRounded(
            (Rectangle){ 
                headX - headSize * 1.2f, 
                headY - headSize * 0.8f, 
                headSize * 0.4f, 
                headSize * 0.6f  // Longer side hair
            },
            0.3f, 8, hairColor
        );
        DrawRectangleRounded(
            (Rectangle){ 
                headX + headSize * 0.8f, 
                headY - headSize * 0.8f, 
                headSize * 0.4f, 
                headSize * 0.6f  // Longer side hair
            },
            0.3f, 8, hairColor
        );
        break;
        
    case 2: // Long hair
        // Top hair band
        DrawRectangleRounded(
            (Rectangle){ 
                headX - headSize * 1.0f, 
                headY - headSize * 1.0f, 
                headSize * 2.0f, 
                headSize * 0.4f  // Thicker hair
            },
            0.3f, 8, hairColor
        );
        
        // Longer side hair extending down
        DrawRectangleRounded(
            (Rectangle){ 
                headX - headSize * 1.2f, 
                headY - headSize * 0.8f, 
                headSize * 0.4f, 
                headSize * 1.0f  // Much longer hair
            },
            0.3f, 8, hairColor
        );
        DrawRectangleRounded(
            (Rectangle){ 
                headX + headSize * 0.8f, 
                headY - headSize * 0.8f, 
                headSize * 0.4f, 
                headSize * 1.0f  // Much longer hair
            },
            0.3f, 8, hairColor
        );
        break;
        
    case 3: // Mohawk
        DrawRectangleRounded(
            (Rectangle){ 
                headX - headSize * 0.2f, 
                headY - headSize * 1.2f, 
                headSize * 0.4f, 
                headSize * 0.6f  // Taller mohawk
            },
            0.3f, 8, hairColor
        );
        break;
        
    case 4: // Bald
        // No hair to draw
        break;
}

// Expanded beard rendering
switch(selectedBeardStyle) {
    case 0: // No beard
        // No beard to draw
        break;

    case 1: // Stubble
        DrawRectangleRounded(
            (Rectangle){ 
                headX - headSize * 0.6f,  // Wider coverage
                headY + headSize * 0.5f,  // Lowered below lips
                headSize * 1.2f, 
                headSize * 0.2f  // Thin stubble
            },
            0.5f, 8, (Color){hairColor.r, hairColor.g, hairColor.b, 100}
        );
        break;
        
    case 2: // Full beard
        DrawRectangleRounded(
            (Rectangle){ 
                headX - headSize * 0.6f,  // Wider coverage
                headY + headSize * 0.5f,  // Lowered below lips
                headSize * 1.2f, 
                headSize * 0.4f  // Thicker beard
            },
            0.5f, 8, hairColor
        );
        break;
        
    case 3: // Goatee
        DrawRectangleRounded(
            (Rectangle){ 
                headX - headSize * 0.3f,  // Centered
                headY + headSize * 0.6f,  // Lowered further
                headSize * 0.6f, 
                headSize * 0.3f  // More proportional
            },
            0.5f, 8, hairColor
        );
        break;
        
    case 4: // Mustache
        DrawRectangleRounded(
            (Rectangle){ 
                headX - headSize * 0.6f,  // Wider coverage
                headY + headSize * 0.4f,  // Just above lips
                headSize * 1.2f, 
                headSize * 0.2f  // Thin mustache
            },
            0.5f, 8, hairColor
        );
        break;
        
    case 5: // Mutton Chops
        // Left side
        DrawRectangleRounded(
            (Rectangle){ 
                headX - headSize * 1.0f,  // Far left
                headY + headSize * 0.5f,  // Below lips
                headSize * 0.4f, 
                headSize * 0.5f  // Thick chop
            },
            0.5f, 8, hairColor
        );
        
        // Right side
        DrawRectangleRounded(
            (Rectangle){ 
                headX + headSize * 0.6f,  // Far right
                headY + headSize * 0.5f,  // Below lips
                headSize * 0.4f, 
                headSize * 0.5f  // Thick chop
            },
            0.5f, 8, hairColor
        );
        break;
        
    case 6: // Handlebar Mustache
        // Mustache base
        DrawRectangleRounded(
            (Rectangle){ 
                headX - headSize * 0.6f,  // Wider coverage
                headY + headSize * 0.4f,  // Just above lips
                headSize * 1.2f, 
                headSize * 0.2f  // Thin mustache base
            },
            0.5f, 8, hairColor
        );
        
        // Left curl
        DrawRectangleRounded(
            (Rectangle){ 
                headX - headSize * 0.8f,  // Extended left
                headY + headSize * 0.3f,  // Raised slightly
                headSize * 0.2f, 
                headSize * 0.2f  // Curl
            },
            0.5f, 8, hairColor
        );
        
        // Right curl
        DrawRectangleRounded(
            (Rectangle){ 
                headX + headSize * 0.6f,  // Extended right
                headY + headSize * 0.3f,  // Raised slightly
                headSize * 0.2f, 
                headSize * 0.2f  // Curl
            },
            0.5f, 8, hairColor
        );
        break;
}


        // If face style is selected, adjust face shape
        switch(selectedFaceStyle) {
            case 0: // Round - already default circle
                break;
            case 1: // Square
                // Draw a square-ish face overlay
                DrawRectangleRounded(
                    (Rectangle){ headX - headSize * 0.8f, headY - headSize * 0.8f, headSize * 1.6f, headSize * 1.6f },
                    0.15f, 8, skinColor
                );
                break;
            case 2: // Oval
                // Draw a stretched oval face overlay
                for (int i = 0; i < 5; i++) {
                    float ovalWidth = headSize * 0.7f;
                    float ovalHeight = headSize * 1.1f;
                    DrawEllipse(
                        headX, headY,
                        ovalWidth - i * 3,
                        ovalHeight - i * 3,
                        skinColor
                    );
                }
                break;
        }
    }
}
//------------------ Pause Menu (Triggered with M) ----------------------
void DrawPauseMenu() {
    float scale = GetScaleFactor();
    Rectangle pauseRect = { (float)(GetScreenWidth()/2 - 150), (float)(GetScreenHeight()/2 - 100), 300, 200 };
    DrawRectangleRec(pauseRect, Fade(BLACK, 0.7f));
    if (GuiButton((Rectangle){ pauseRect.x + 50, pauseRect.y + 30, 200, 40 }, "Resume"))
        isPaused = false;
    if (GuiButton((Rectangle){ pauseRect.x + 50, pauseRect.y + 80, 200, 40 }, "Main Menu")) {
        isPaused = false;
        gameState = MAIN_MENU;
    }
    if (GuiButton((Rectangle){ pauseRect.x + 50, pauseRect.y + 130, 200, 40 }, "Quit"))
        CloseWindow();
}

//------------------ Draw Attribute Bar Helper Function ----------------------
void DrawAttributeBar(float x, float y, float width, float height, int value, int maxValue, Color color) {
    DrawRectangleRec((Rectangle){x, y, width, height}, LIGHTGRAY);
    DrawRectangleRec((Rectangle){x, y, width * ((float)value / maxValue), height}, color);
    DrawRectangleLinesEx((Rectangle){x, y, width, height}, 1, BLACK);
}

//------------------ Level Complete Screen ----------------------
void DrawLevelComplete() {
    float scale = GetScaleFactor();
    ClearBackground((Color){10, 5, 30, 255}); // Space background
    
    // Create space effect
    for (int i = 0; i < 100; i++) {
        float x = (float)(GetRandomValue(0, GetScreenWidth()));
        float y = (float)(GetRandomValue(0, GetScreenHeight()));
        float size = (float)(GetRandomValue(1, 3));
        DrawCircle(x, y, size, WHITE);
    }
    
    // Draw completion message
    DrawTextEx(customFont, "LEVEL COMPLETE!", (Vector2){(float)(GetScreenWidth()/2 - 200 * scale), 150 * scale}, 50 * scale, 2, WHITE);
    
    // Draw stats
    DrawRectangleRounded(
        (Rectangle){(float)(GetScreenWidth()/2 - 200 * scale), 230 * scale, 400 * scale, 220 * scale},
        0.1f, 8, (Color){20, 20, 50, 200}
    );
    
    DrawTextEx(customFont, TextFormat("Score: %d", player.score), (Vector2){(float)(GetScreenWidth()/2 - 150 * scale), 250 * scale}, 30 * scale, 2, WHITE);
    DrawTextEx(customFont, TextFormat("Coins Collected: %d", player.currency), (Vector2){(float)(GetScreenWidth()/2 - 150 * scale), 300 * scale}, 30 * scale, 2, WHITE);
    DrawTextEx(customFont, TextFormat("Completion Bonus: %d", levelCompletionBonus), (Vector2){(float)(GetScreenWidth()/2 - 150 * scale), 350 * scale}, 30 * scale, 2, WHITE);
    DrawTextEx(customFont, TextFormat("Total Currency: %d", player.currency), (Vector2){(float)(GetScreenWidth()/2 - 150 * scale), 400 * scale}, 30 * scale, 2, GOLD);
    
    // Draw buttons
    if (GuiButton((Rectangle){(float)(GetScreenWidth()/2 - 100 * scale), 470 * scale, 200 * scale, 50 * scale}, "Next Level"))
    {
        gameState = PLATFORMER;
        InitPlatformerLevel(levelExit.targetLevel);
    }
    
    if (GuiButton((Rectangle){(float)(GetScreenWidth()/2 - 100 * scale), 540 * scale, 200 * scale, 50 * scale}, "Main Menu"))
    {
        playerCurrency = player.currency; // Save currency
        gameState = MAIN_MENU;
    }
    
    // Draw player character as decoration
    DrawDetailedCharacter((float)(GetScreenWidth()/2 + 250 * scale), 350 * scale, scale * 1.2f, true);
}

//------------------ Menus ----------------------
void DrawMainMenu() {
    float scale = GetScaleFactor();
    
    // Draw space background
    DrawDetailedSpace(0);
    
    // Menu panel
    DrawRectangleRounded(
        (Rectangle){400 * scale, 80 * scale, 480 * scale, 500 * scale},
        0.1f, 8, (Color){20, 20, 50, 200}
    );
    
    DrawTextEx(customFont, "SPACE VENTURE v2.0", (Vector2){500 * scale, 100 * scale}, 50 * scale, 2, WHITE);
    
    // Display player's total currency
    DrawTextEx(customFont, TextFormat("Credits: %d", playerCurrency), (Vector2){500 * scale, 170 * scale}, 30 * scale, 2, GOLD);
    
    if (GuiButton((Rectangle){500 * scale, 250 * scale, 280 * scale, 50 * scale}, "New Game"))
        gameState = CHARACTER_CREATION;
    if (GuiButton((Rectangle){500 * scale, 320 * scale, 280 * scale, 50 * scale}, "Settings"))
        gameState = SETTINGS;
    if (GuiButton((Rectangle){500 * scale, 390 * scale, 280 * scale, 50 * scale}, "Spaceship Combat")) {
        gameState = SPACESHIP_COMBAT;
        // InitSpaceCombatLevel();
    }
    if (GuiButton((Rectangle){500 * scale, 460 * scale, 280 * scale, 50 * scale}, "Quit"))
        CloseWindow();
        
    // Draw player character as decoration
    DrawDetailedCharacter(300 * scale, 400 * scale, scale, true);
}

void DrawSettingsMenu() {
    float scale = GetScaleFactor();
    
    // Draw space background
    DrawDetailedSpace(0);
    
    // Settings panel
    DrawRectangleRounded(
        (Rectangle){400 * scale, 80 * scale, 480 * scale, 550 * scale},
        0.1f, 8, (Color){20, 20, 50, 200}
    );
    
    DrawTextEx(customFont, "Settings", (Vector2){550 * scale, 100 * scale}, 50 * scale, 2, WHITE);
    DrawTextEx(customFont, "Resolution:", (Vector2){500 * scale, 200 * scale}, 30 * scale, 2, WHITE);
    if (GuiButton((Rectangle){500 * scale, 250 * scale, 280 * scale, 50 * scale}, resolutions[selectedResolution])) {
        selectedResolution = (selectedResolution + 1) % 3;
        switch (selectedResolution) {
            case 0: SetWindowSize(1280, 720); screenWidth = 1280; screenHeight = 720; break;
            case 1: SetWindowSize(1920, 1080); screenWidth = 1920; screenHeight = 1080; break;
            case 2: SetWindowSize(2560, 1440); screenWidth = 2560; screenHeight = 1440; break;
        }
    }
    if (GuiButton((Rectangle){500 * scale, 320 * scale, 280 * scale, 50 * scale}, "Fullscreen")) {
        ToggleFullscreen();
        switch (selectedResolution) {
            case 0: SetWindowSize(1280, 720); break;
            case 1: SetWindowSize(1920, 1080); break;
            case 2: SetWindowSize(2560, 1440); break;
        }
    }
    DrawTextEx(customFont, "Music Volume:", (Vector2){500 * scale, 400 * scale}, 30 * scale, 2, WHITE);
    if (GuiSlider((Rectangle){500 * scale, 450 * scale, 280 * scale, 50 * scale}, "", "", &musicVolume, 0.0f, 1.0f))
        SetMusicVolume(musicVolume);
    if (GuiButton((Rectangle){500 * scale, 520 * scale, 280 * scale, 50 * scale}, isMusicPaused ? "Unpause Music" : "Pause Music"))
        ToggleMusicPause();
    if (GuiButton((Rectangle){500 * scale, 590 * scale, 280 * scale, 50 * scale}, "Back"))
        gameState = MAIN_MENU;
}

void DrawCharacterCreation() {
    float scale = GetScaleFactor();
    
    // Draw space background
    DrawDetailedSpace(0);
    
    // Creation panel
    DrawRectangleRounded(
        (Rectangle){300 * scale, 150 * scale, 680 * scale, 400 * scale},
        0.1f, 8, (Color){20, 20, 50, 200}
    );
    
    DrawTextEx(customFont, "Enter Character Name:", (Vector2){400 * scale, 200 * scale}, 30 * scale, 2, WHITE);
    DrawRectangle(400 * scale, 250 * scale, 400 * scale, 50 * scale, (Color){40, 40, 70, 255});
    DrawTextEx(customFont, nameInput, (Vector2){410 * scale, 260 * scale}, 30 * scale, 2, WHITE);
    int key = GetCharPressed();
    while (key > 0) {
        if ((key >= 32) && (key <= 125) && (nameIndex < 19)) {
            nameInput[nameIndex++] = (char)key;
            nameInput[nameIndex] = '\0';
        }
        key = GetCharPressed();
    }
    if (IsKeyPressed(KEY_BACKSPACE) && nameIndex > 0)
        nameInput[--nameIndex] = '\0';
    if (GuiButton((Rectangle){500 * scale, 400 * scale, 280 * scale, 50 * scale}, "Start Game")) {
        playerName = nameInput;
        gameState = CHARACTER_CUSTOMIZATION;
        // Make sure helmet is off in customization screen
        hasHelmet = false;
    }
}

void DrawCharacterCustomization() {
    float scale = GetScaleFactor();
    
    // Draw space background
    DrawDetailedSpace(0);
    
    // Main panel
    DrawRectangleRounded(
        (Rectangle){250 * scale, 50 * scale, 780 * scale, 620 * scale},
        0.1f, 8, (Color){20, 20, 50, 200}
    );
    
    DrawTextEx(customFont, "Character Customization", (Vector2){400 * scale, 60 * scale}, 50 * scale, 2, WHITE);
    Rectangle tabAppearance = {300 * scale, 120 * scale, 150 * scale, 40 * scale};
    Rectangle tabAttributes = {450 * scale, 120 * scale, 150 * scale, 40 * scale};
    Rectangle tabEquipment = {600 * scale, 120 * scale, 150 * scale, 40 * scale};
    if (GuiButton(tabAppearance, "Appearance")) currentTab = TAB_APPEARANCE;
    if (GuiButton(tabAttributes, "Attributes")) currentTab = TAB_ATTRIBUTES;
    if (GuiButton(tabEquipment, "Equipment")) currentTab = TAB_EQUIPMENT;
    Rectangle activeTabIndicator;
    switch(currentTab) {
        case TAB_APPEARANCE: activeTabIndicator = tabAppearance; break;
        case TAB_ATTRIBUTES: activeTabIndicator = tabAttributes; break;
        case TAB_EQUIPMENT: activeTabIndicator = tabEquipment; break;
    }
    DrawRectangleLines(activeTabIndicator.x, activeTabIndicator.y, activeTabIndicator.width, activeTabIndicator.height, RED);
    DrawRectangle(300 * scale, 170 * scale, 450 * scale, 400 * scale, (Color){30, 30, 60, 200}); // Options panel
    DrawRectangle(800 * scale, 170 * scale, 300 * scale, 400 * scale, (Color){40, 40, 70, 200});  // Preview panel
    DrawTextEx(customFont, "Character Preview", (Vector2){850 * scale, 180 * scale}, 20 * scale, 2, WHITE);
    
    // Draw character preview - without helmet in customization
    DrawDetailedCharacter(950 * scale, 370 * scale, scale * 1.5f, false);
    
    if (currentTab == TAB_APPEARANCE) {
         // Add spacesuit selection
         DrawTextEx(customFont, "Spacesuit:", (Vector2){320 * scale, 190 * scale}, 25 * scale, 2, WHITE);
         if (GuiButton((Rectangle){500 * scale, 190 * scale, 200 * scale, 30 * scale}, playerAppearanceNames[selectedPlayerAppearance]))
              selectedPlayerAppearance = (selectedPlayerAppearance + 1) % 3;
         
         // Hair customization options
         DrawTextEx(customFont, "Hairstyle:", (Vector2){320 * scale, 230 * scale}, 25 * scale, 2, WHITE);
         if (GuiButton((Rectangle){500 * scale, 230 * scale, 200 * scale, 30 * scale}, hairstyles[selectedHairstyle]))
              selectedHairstyle = (selectedHairstyle + 1) % 5;
         DrawTextEx(customFont, "Hair Color:", (Vector2){320 * scale, 270 * scale}, 25 * scale, 2, WHITE);
         if (GuiButton((Rectangle){500 * scale, 270 * scale, 200 * scale, 30 * scale}, hairColors[selectedHairColor]))
              selectedHairColor = (selectedHairColor + 1) % 5;
              
         // Beard options
         DrawTextEx(customFont, "Beard Style:", (Vector2){320 * scale, 310 * scale}, 25 * scale, 2, WHITE);
         if (GuiButton((Rectangle){500 * scale, 310 * scale, 200 * scale, 30 * scale}, beardStyles[selectedBeardStyle]))
              selectedBeardStyle = (selectedBeardStyle + 1) % 4;
         
         // Skin and face options
         DrawTextEx(customFont, "Skin Color:", (Vector2){320 * scale, 350 * scale}, 25 * scale, 2, WHITE);
         if (GuiButton((Rectangle){500 * scale, 350 * scale, 200 * scale, 30 * scale}, skinColors[selectedSkinColor]))
              selectedSkinColor = (selectedSkinColor + 1) % 3;
         DrawTextEx(customFont, "Eye Color:", (Vector2){320 * scale, 390 * scale}, 25 * scale, 2, WHITE);
         if (GuiButton((Rectangle){500 * scale, 390 * scale, 200 * scale, 30 * scale}, eyeColors[selectedEyeColor]))
              selectedEyeColor = (selectedEyeColor + 1) % 4;
         DrawTextEx(customFont, "Face Shape:", (Vector2){320 * scale, 430 * scale}, 25 * scale, 2, WHITE);
         if (GuiButton((Rectangle){500 * scale, 430 * scale, 200 * scale, 30 * scale}, faceStyles[selectedFaceStyle]))
              selectedFaceStyle = (selectedFaceStyle + 1) % 3;
              
         // Add appearance description
         DrawRectangle(320 * scale, 470 * scale, 380 * scale, 80 * scale, (Color){40, 40, 70, 200});
         switch(selectedPlayerAppearance) {
             case 0:
                 DrawTextEx(customFont, "Standard Spacesuit", (Vector2){330 * scale, 480 * scale}, 20 * scale, 2, WHITE);
                 DrawTextEx(customFont, "All-purpose suit with balanced protection", (Vector2){330 * scale, 510 * scale}, 18 * scale, 2, LIGHTGRAY);
                 break;
             case 1:
                 DrawTextEx(customFont, "Tactical Spacesuit", (Vector2){330 * scale, 480 * scale}, 20 * scale, 2, WHITE);
                 DrawTextEx(customFont, "Enhanced mobility and weapon stabilization", (Vector2){330 * scale, 510 * scale}, 18 * scale, 2, LIGHTGRAY);
                 break;
             case 2:
                 DrawTextEx(customFont, "Elite Spacesuit", (Vector2){330 * scale, 480 * scale}, 20 * scale, 2, WHITE);
                 DrawTextEx(customFont, "Superior armor and life support systems", (Vector2){330 * scale, 510 * scale}, 18 * scale, 2, LIGHTGRAY);
                 break;
         }
    } else if (currentTab == TAB_ATTRIBUTES) {
         // Class selection with enhanced description
         DrawTextEx(customFont, "Class:", (Vector2){320 * scale, 190 * scale}, 25 * scale, 2, WHITE);
         if (GuiButton((Rectangle){500 * scale, 190 * scale, 200 * scale, 30 * scale}, fightingClasses[selectedFightingClass]))
              selectedFightingClass = (selectedFightingClass + 1) % 3;
              
         // Class description box
         DrawRectangle(320 * scale, 230 * scale, 380 * scale, 80 * scale, (Color){40, 40, 70, 200});
         switch(selectedFightingClass) {
             case 0: // Expert Pilot
                 DrawTextEx(customFont, "Expert Pilot", (Vector2){330 * scale, 240 * scale}, 20 * scale, 2, WHITE);
                 DrawTextEx(customFont, "Skilled in spacecraft navigation", (Vector2){330 * scale, 265 * scale}, 18 * scale, 2, LIGHTGRAY);
                 DrawTextEx(customFont, "Bonus: +2 Agility, +1 Intelligence", (Vector2){330 * scale, 290 * scale}, 18 * scale, 2, LIGHTGRAY);
                 break;
             case 1: // Soldier
                 DrawTextEx(customFont, "Soldier", (Vector2){330 * scale, 240 * scale}, 20 * scale, 2, WHITE);
                 DrawTextEx(customFont, "Combat specialist with heavy weapons", (Vector2){330 * scale, 265 * scale}, 18 * scale, 2, LIGHTGRAY);
                 DrawTextEx(customFont, "Bonus: +3 Strength, +1 Health", (Vector2){330 * scale, 290 * scale}, 18 * scale, 2, LIGHTGRAY);
                 break;
             case 2: // Hacker
                 DrawTextEx(customFont, "Hacker", (Vector2){330 * scale, 240 * scale}, 20 * scale, 2, WHITE);
                 DrawTextEx(customFont, "Expert in technology and systems", (Vector2){330 * scale, 265 * scale}, 18 * scale, 2, LIGHTGRAY);
                 DrawTextEx(customFont, "Bonus: +3 Intelligence, +1 Energy", (Vector2){330 * scale, 290 * scale}, 18 * scale, 2, LIGHTGRAY);
                 break;
         }
         
         // Attribute points with visual bars
         DrawTextEx(customFont, TextFormat("Attribute Points: %d", totalAttributePoints), 
                   (Vector2){320 * scale, 320 * scale}, 25 * scale, 2, WHITE);
         
         // Strength
         DrawTextEx(customFont, "Strength:", (Vector2){320 * scale, 360 * scale}, 22 * scale, 2, WHITE);
         DrawAttributeBar(500 * scale, 360 * scale, 150 * scale, 20 * scale, strengthPoints, 10, RED);
         DrawTextEx(customFont, TextFormat("%d", strengthPoints), (Vector2){660 * scale, 360 * scale}, 22 * scale, 2, WHITE);
         if (totalAttributePoints > 0) {
             if (GuiButton((Rectangle){690 * scale, 360 * scale, 30 * scale, 20 * scale}, "+")) {
                 strengthPoints++;
                 totalAttributePoints--;
             }
         }
         if (strengthPoints > 5) {
             if (GuiButton((Rectangle){725 * scale, 360 * scale, 30 * scale, 20 * scale}, "-")) {
                 strengthPoints--;
                 totalAttributePoints++;
             }
         }
         
         // Agility
         DrawTextEx(customFont, "Agility:", (Vector2){320 * scale, 390 * scale}, 22 * scale, 2, WHITE);
         DrawAttributeBar(500 * scale, 390 * scale, 150 * scale, 20 * scale, agilityPoints, 10, GREEN);
         DrawTextEx(customFont, TextFormat("%d", agilityPoints), (Vector2){660 * scale, 390 * scale}, 22 * scale, 2, WHITE);
         if (totalAttributePoints > 0) {
             if (GuiButton((Rectangle){690 * scale, 390 * scale, 30 * scale, 20 * scale}, "+")) {
                 agilityPoints++;
                 totalAttributePoints--;
             }
         }
         if (agilityPoints > 5) {
             if (GuiButton((Rectangle){725 * scale, 390 * scale, 30 * scale, 20 * scale}, "-")) {
                 agilityPoints--;
                 totalAttributePoints++;
             }
         }
         
         // Intelligence
         DrawTextEx(customFont, "Intelligence:", (Vector2){320 * scale, 420 * scale}, 22 * scale, 2, WHITE);
         DrawAttributeBar(500 * scale, 420 * scale, 150 * scale, 20 * scale, intelligencePoints, 10, BLUE);
         DrawTextEx(customFont, TextFormat("%d", intelligencePoints), (Vector2){660 * scale, 420 * scale}, 22 * scale, 2, WHITE);
         if (totalAttributePoints > 0) {
             if (GuiButton((Rectangle){690 * scale, 420 * scale, 30 * scale, 20 * scale}, "+")) {
                 intelligencePoints++;
                 totalAttributePoints--;
             }
         }
         if (intelligencePoints > 5) {
             if (GuiButton((Rectangle){725 * scale, 420 * scale, 30 * scale, 20 * scale}, "-")) {
                 intelligencePoints--;
                 totalAttributePoints++;
             }
         }
         
         // Class effect description
         DrawRectangle(320 * scale, 460 * scale, 380 * scale, 60 * scale, (Color){40, 40, 70, 200});
         DrawTextEx(customFont, "Class Effects:", (Vector2){330 * scale, 470 * scale}, 20 * scale, 2, WHITE);
         
         if (selectedFightingClass == 0) { // Expert Pilot
             DrawTextEx(customFont, "Ship controls more responsive", (Vector2){330 * scale, 495 * scale}, 18 * scale, 2, LIGHTGRAY);
         } else if (selectedFightingClass == 1) { // Soldier
             DrawTextEx(customFont, "Weapon damage increased by 25%", (Vector2){330 * scale, 495 * scale}, 18 * scale, 2, LIGHTGRAY);
         } else if (selectedFightingClass == 2) { // Hacker
             DrawTextEx(customFont, "Can disable enemy systems temporarily", (Vector2){330 * scale, 495 * scale}, 18 * scale, 2, LIGHTGRAY);
         }
    } else if (currentTab == TAB_EQUIPMENT) {
         DrawTextEx(customFont, "Weapon:", (Vector2){320 * scale, 190 * scale}, 25 * scale, 2, WHITE);
         if (GuiButton((Rectangle){500 * scale, 190 * scale, 200 * scale, 30 * scale}, weapons[selectedWeapon]))
              selectedWeapon = (selectedWeapon + 1) % 3;
         DrawTextEx(customFont, "Armor:", (Vector2){320 * scale, 230 * scale}, 25 * scale, 2, WHITE);
         if (GuiButton((Rectangle){500 * scale, 230 * scale, 200 * scale, 30 * scale}, armors[selectedArmor]))
              selectedArmor = (selectedArmor + 1) % 3;
         DrawTextEx(customFont, "Accessory:", (Vector2){320 * scale, 270 * scale}, 25 * scale, 2, WHITE);
         if (GuiButton((Rectangle){500 * scale, 270 * scale, 200 * scale, 30 * scale}, accessories[selectedAccessory]))
              selectedAccessory = (selectedAccessory + 1) % 3;
              
         // Enhanced weapon description
         DrawRectangle(320 * scale, 320 * scale, 380 * scale, 70 * scale, (Color){40, 40, 70, 200});
         DrawTextEx(customFont, "Weapon Stats:", (Vector2){330 * scale, 330 * scale}, 22 * scale, 2, WHITE);
         switch(selectedWeapon) {
                case 0: 
                    DrawTextEx(customFont, "Blaster Pistol", (Vector2){330 * scale, 355 * scale}, 20 * scale, 2, WHITE);
                    DrawTextEx(customFont, "DMG: 5 | SPD: Fast | RNG: Medium", (Vector2){330 * scale, 380 * scale}, 18 * scale, 2, LIGHTGRAY); 
                    break;
                case 1: 
                    DrawTextEx(customFont, "Plasma Rifle", (Vector2){330 * scale, 355 * scale}, 20 * scale, 2, WHITE);
                    DrawTextEx(customFont, "DMG: 8 | SPD: Medium | RNG: Long", (Vector2){330 * scale, 380 * scale}, 18 * scale, 2, LIGHTGRAY); 
                    break;
                case 2: 
                    DrawTextEx(customFont, "Neural Disruptor", (Vector2){330 * scale, 355 * scale}, 20 * scale, 2, WHITE);
                    DrawTextEx(customFont, "DMG: 12 | SPD: Slow | RNG: Short", (Vector2){330 * scale, 380 * scale}, 18 * scale, 2, LIGHTGRAY); 
                    break;
         }
         
         // Enhanced armor description
         DrawRectangle(320 * scale, 400 * scale, 380 * scale, 70 * scale, (Color){40, 40, 70, 200});
         DrawTextEx(customFont, "Armor Stats:", (Vector2){330 * scale, 410 * scale}, 22 * scale, 2, WHITE);
         switch(selectedArmor) {
                case 0:
                    DrawTextEx(customFont, "Stealth Suit", (Vector2){330 * scale, 435 * scale}, 20 * scale, 2, WHITE);
                    DrawTextEx(customFont, "DEF: 3 | AGI: +2 | SNEAK: High", (Vector2){330 * scale, 460 * scale}, 18 * scale, 2, LIGHTGRAY);
                    break;
                case 1:
                    DrawTextEx(customFont, "Combat Armor", (Vector2){330 * scale, 435 * scale}, 20 * scale, 2, WHITE);
                    DrawTextEx(customFont, "DEF: 7 | AGI: +0 | SNEAK: Low", (Vector2){330 * scale, 460 * scale}, 18 * scale, 2, LIGHTGRAY);
                    break;
                case 2:
                    DrawTextEx(customFont, "Power Exoskeleton", (Vector2){330 * scale, 435 * scale}, 20 * scale, 2, WHITE);
                    DrawTextEx(customFont, "DEF: 10 | AGI: -1 | SNEAK: None", (Vector2){330 * scale, 460 * scale}, 18 * scale, 2, LIGHTGRAY);
                    break;
         }
         
         // Accessory description
         DrawRectangle(320 * scale, 480 * scale, 380 * scale, 70 * scale, (Color){40, 40, 70, 200});
         DrawTextEx(customFont, "Accessory Effect:", (Vector2){330 * scale, 490 * scale}, 22 * scale, 2, WHITE);
         switch(selectedAccessory) {
                case 0:
                    DrawTextEx(customFont, "Wrist Computer: Improves hacking", (Vector2){330 * scale, 515 * scale}, 18 * scale, 2, LIGHTGRAY);
                    break;
                case 1:
                    DrawTextEx(customFont, "Neural Implant: Enhances reflexes", (Vector2){330 * scale, 515 * scale}, 18 * scale, 2, LIGHTGRAY);
                    break;
                case 2:
                    DrawTextEx(customFont, "Holographic Badge: Access to elite areas", (Vector2){330 * scale, 515 * scale}, 18 * scale, 2, LIGHTGRAY);
                    break;
         }
    }
    
    if (GuiButton((Rectangle){400 * scale, 580 * scale, 150 * scale, 50 * scale}, "Back"))
         gameState = CHARACTER_CREATION;
    if (GuiButton((Rectangle){600 * scale, 580 * scale, 150 * scale, 50 * scale}, "Start Game"))
         TransitionToGameplay();
}

void DrawPlaying() {
    ClearBackground(BLACK);
    DrawTextEx(customFont, "Playing State", (Vector2){20, 20}, 40, 2, WHITE);
}

//------------------ Update Platformer (with Pause via M) ----------------------
void UpdatePlatformer() {
    if (IsKeyPressed(KEY_M))
        isPaused = !isPaused;
    if (isPaused)
        return;
    
    // Player movement controls
    if (IsKeyDown(KEY_RIGHT)) { player.velocity.x = MOVE_SPEED; player.facingRight = true; }
    else if (IsKeyDown(KEY_LEFT)) { player.velocity.x = -MOVE_SPEED; player.facingRight = false; }
    else { player.velocity.x = 0; }
    
    player.velocity.y += GRAVITY;
    if (IsKeyPressed(KEY_UP) && player.canJump) {
        player.velocity.y = JUMP_FORCE;
        player.isJumping = true;
        player.canJump = false;
        PlaySound(jumpSound);
    }
    player.rect.x += player.velocity.x;
    player.rect.y += player.velocity.y;
    
    // Platform collision
    player.canJump = false;
    for (auto& platform : platforms) {
        Rectangle playerFeet = { player.rect.x, player.rect.y + player.rect.height - 5, player.rect.width, 10 };
        if (CheckCollisionRecs(playerFeet, platform.rect)) {
            if (player.velocity.y > 0) {
                player.rect.y = platform.rect.y - player.rect.height;
                player.velocity.y = 0;
                player.isJumping = false;
                player.canJump = true;
                if (platform.deadly) {
                    player.health -= 10;
                    PlaySound(hitSound);
                    player.velocity.y = -8.0f;
                }
                if (platform.type == 2)
                    platform.rect.x = -100; // Remove breakable platform
            }
        }
        if (platform.type == 1) {
            // Handle both horizontal and vertical moving platforms
            platform.rect.x += platform.velocity.x;
            platform.rect.y += platform.velocity.y;
            
            // Bounce horizontal platforms
            if (platform.velocity.x != 0 && 
                (platform.rect.x < 0 || platform.rect.x > levelBounds.width - platform.rect.width)) {
                platform.velocity.x *= -1;
            }
            
            // Bounce vertical platforms (shorter range)
            if (platform.velocity.y != 0) {
                float originalY = platform.rect.y - platform.velocity.y; // Get original position before this move
                float moveRange = 100.0f; // Range of vertical movement
                
                if ((platform.velocity.y > 0 && platform.rect.y > originalY + moveRange) ||
                    (platform.velocity.y < 0 && platform.rect.y < originalY - moveRange)) {
                    platform.velocity.y *= -1;
                }
            }
            
            // Move player along with platform if standing on it
            if (player.canJump && CheckCollisionRecs(playerFeet, platform.rect)) {
                player.rect.x += platform.velocity.x;
                // Don't move player vertically with platform - feels weird in gameplay
            }
        }
    }
    
    // Projectile shooting
    if (IsKeyPressed(KEY_SPACE)) {
        float projectileX = player.facingRight ? player.rect.x + player.rect.width : player.rect.x;
        float projectileY = player.rect.y + player.rect.height / 2;
        float velocity = player.facingRight ? 10.0f : -10.0f;
        int damage = (selectedWeapon == 0) ? 1 : (selectedWeapon == 1) ? 2 : 3;
        if(selectedWeapon == 0) velocity = player.facingRight ? 15.0f : -15.0f;
        else if(selectedWeapon == 1) velocity = player.facingRight ? 12.0f : -12.0f;
        else if(selectedWeapon == 2) velocity = player.facingRight ? 8.0f : -8.0f;
        ShootProjectile(projectileX, projectileY, velocity, true, damage);
    }
    
    // Keep player in bounds
    if (player.rect.x < 0) player.rect.x = 0;
    if (player.rect.x > levelBounds.width - player.rect.width)
        player.rect.x = levelBounds.width - player.rect.width;
    
    // Enemy updates
    for (auto& enemy : enemies) {
        if (!enemy.active) continue;
        switch(enemy.type) {
            case 0: // Basic enemy
                enemy.rect.x += enemy.velocity.x;
                if (enemy.rect.x < 0 || enemy.rect.x > levelBounds.width - enemy.rect.width) {
                    enemy.velocity.x *= -1;
                    enemy.facingRight = !enemy.facingRight;
                }
                enemy.velocity.y += GRAVITY;
                enemy.rect.y += enemy.velocity.y;
                for (auto& platform : platforms) {
                    Rectangle enemyFeet = { enemy.rect.x, enemy.rect.y + enemy.rect.height - 5, enemy.rect.width, 10 };
                    if (CheckCollisionRecs(enemyFeet, platform.rect)) {
                        if (enemy.velocity.y > 0) {
                            enemy.rect.y = platform.rect.y - enemy.rect.height;
                            enemy.velocity.y = 0;
                        }
                    }
                }
                enemy.timer += GetFrameTime();
                if (enemy.timer > 3.0f) {
                    float projectileX = enemy.facingRight ? enemy.rect.x + enemy.rect.width : enemy.rect.x;
                    float projectileY = enemy.rect.y + enemy.rect.height / 2;
                    float velocity = enemy.facingRight ? 8.0f : -8.0f;
                    ShootProjectile(projectileX, projectileY, velocity, false, 1);
                    enemy.timer = 0;
                }
                break;
          // Continuing from previous code block...

            case 1: // Flying enemy
                enemy.timer += GetFrameTime();
                enemy.rect.x += enemy.velocity.x;
                enemy.rect.y = enemy.rect.y + sinf(enemy.timer * 2) * 2;
                if (enemy.rect.x < 0 || enemy.rect.x > levelBounds.width - enemy.rect.width) {
                    enemy.velocity.x *= -1;
                    enemy.facingRight = !enemy.facingRight;
                }
                if (enemy.timer > 2.0f) {
                    float projectileX = enemy.facingRight ? enemy.rect.x + enemy.rect.width : enemy.rect.x;
                    float projectileY = enemy.rect.y + enemy.rect.height / 2;
                    float velocity = enemy.facingRight ? 8.0f : -8.0f;
                    ShootProjectile(projectileX, projectileY, velocity, false, 1);
                    enemy.timer = 0;
                }
                break;
            case 2: // Heavy enemy
                enemy.rect.x += enemy.velocity.x;
                if (enemy.rect.x < 0 || enemy.rect.x > levelBounds.width - enemy.rect.width) {
                    enemy.velocity.x *= -1;
                    enemy.facingRight = !enemy.facingRight;
                }
                enemy.velocity.y += GRAVITY;
                enemy.rect.y += enemy.velocity.y;
                for (auto& platform : platforms) {
                    Rectangle enemyFeet = { enemy.rect.x, enemy.rect.y + enemy.rect.height - 5, enemy.rect.width, 10 };
                    if (CheckCollisionRecs(enemyFeet, platform.rect)) {
                        if (enemy.velocity.y > 0) {
                            enemy.rect.y = platform.rect.y - enemy.rect.height;
                            enemy.velocity.y = 0;
                        }
                    }
                }
                enemy.timer += GetFrameTime();
                if (enemy.timer > 4.0f) {
                    float projectileX = enemy.facingRight ? enemy.rect.x + enemy.rect.width : enemy.rect.x;
                    float projectileY = enemy.rect.y + enemy.rect.height / 2;
                    float velocity = enemy.facingRight ? 6.0f : -6.0f;
                    ShootProjectile(projectileX, projectileY, velocity, false, 2);
                    enemy.timer = 0;
                }
                break;
        }
        
        // Enemy-player collision
        if (CheckCollisionRecs(player.rect, enemy.rect)) {
            player.health -= 5;
            PlaySound(hitSound);
            player.velocity.x = player.rect.x < enemy.rect.x ? -8.0f : 8.0f;
            player.velocity.y = -5.0f;
        }
    }
    
    // Projectile updates
    for (auto& proj : projectiles) {
        if (!proj.active) continue;
        proj.rect.x += proj.velocity.x;
        if (proj.rect.x < 0 || proj.rect.x > levelBounds.width) { proj.active = false; continue; }
        if (CheckCollisionWithPlatforms(proj.rect)) { proj.active = false; continue; }
        if (!proj.fromPlayer && CheckCollisionRecs(proj.rect, player.rect)) {
            player.health -= proj.damage;
            proj.active = false;
            PlaySound(hitSound);
            continue;
        }
        if (proj.fromPlayer) {
            for (auto& enemy : enemies) {
                if (!enemy.active) continue;
                if (CheckCollisionRecs(proj.rect, enemy.rect)) {
                    enemy.health -= proj.damage;
                    proj.active = false;
                    PlaySound(hitSound);
                    if (enemy.health <= 0) {
                        enemy.active = false;
                        player.score += 100 * (enemy.type + 1);
                        player.currency += enemy.currencyValue; // Award currency for defeating enemies
                    }
                    break;
                }
            }
        }
    }
    
    // Collectible updates
    for (auto& collectible : collectibles) {
        if (!collectible.active) continue;
        
        if (CheckCollisionRecs(player.rect, collectible.rect)) {
            if (collectible.type == 0) { // Coin
                player.currency += collectible.value;
                PlaySound(coinSound);
            } else if (collectible.type == 1) { // Health
                player.health = std::min(player.health + collectible.value, playerMaxHealth);
                PlaySound(coinSound);
            } else if (collectible.type == 2) { // Powerup
                // Apply powerup effect (e.g., temporary invincibility, speed boost)
                player.score += collectible.value * 10;
                PlaySound(coinSound);
            }
            collectible.active = false;
        }
    }
    
    // Check for level exit
    if (levelExit.active && CheckCollisionRecs(player.rect, levelExit.rect)) {
        PlaySound(portalSound);
        TransitionToNextLevel();
    }
    
    // Camera follows player
    float targetCameraX = player.rect.x - GetScreenWidth() / 2 + player.rect.width / 2;
    if (targetCameraX < 0) targetCameraX = 0;
    if (targetCameraX > levelBounds.width - GetScreenWidth())
        targetCameraX = levelBounds.width - GetScreenWidth();
    cameraOffset.x = targetCameraX;
    
    // Check for player death
    if (player.health <= 0) gameState = MAIN_MENU;
    
    // Update score
    playerScore = player.score;
    playerCurrency = player.currency;
    
    // Clean up inactive objects
    collectibles.erase(
        std::remove_if(collectibles.begin(), collectibles.end(), [](Collectible c){ return !c.active; }),
        collectibles.end()
    );
    
    projectiles.erase(
        std::remove_if(projectiles.begin(), projectiles.end(), [](Projectile p){ return !p.active; }),
        projectiles.end()
    );
}

void DrawPlatformer() {
    BeginMode2D((Camera2D){
        .offset = {0, 0},
        .target = { cameraOffset.x, 0 },
        .rotation = 0.0f,
        .zoom = 1.0f
    });
    
    // Draw space background
    DrawDetailedSpace(cameraOffset.x);
    
    // Draw platforms with programmatically generated graphics
    for (auto& platform : platforms) {
        if (platform.deadly) {
            // Draw spikes
            DrawSpikes(platform.rect.x, platform.rect.y, platform.rect.width, platform.rect.height);
        } else {
            // Draw platform based on type
            Color platformColor;
            if (platform.type == 0) {
                platformColor = (Color){150, 150, 200, 255}; // Basic platform
            } else if (platform.type == 1) {
                platformColor = (Color){100, 200, 150, 255}; // Moving platform
            } else { // type == 2
                platformColor = (Color){200, 150, 100, 255}; // Breakable platform
            }
            
            DrawRectangleRounded(platform.rect, 0.2f, 8, platformColor);
            
            // Add platform details
            float stripeWidth = platform.rect.width / 10.0f;
            for (int i = 0; i < 10; i += 2) {
                DrawRectangle(
                    platform.rect.x + i * stripeWidth,
                    platform.rect.y + platform.rect.height * 0.7f,
                    stripeWidth,
                    platform.rect.height * 0.3f,
                    (Color){
                        (unsigned char)(platformColor.r * 0.8f),
                        (unsigned char)(platformColor.g * 0.8f),
                        (unsigned char)(platformColor.b * 0.8f),
                        255
                    }
                );
            }
            
            // Add highlight to top of platform
            DrawRectangle(
                platform.rect.x,
                platform.rect.y,
                platform.rect.width,
                platform.rect.height * 0.2f,
                (Color){
                    (unsigned char)(platformColor.r * 1.2f > 255 ? 255 : platformColor.r * 1.2f),
                    (unsigned char)(platformColor.g * 1.2f > 255 ? 255 : platformColor.g * 1.2f),
                    (unsigned char)(platformColor.b * 1.2f > 255 ? 255 : platformColor.b * 1.2f),
                    255
                }
            );
            
            // Add special effects for moving or breakable platforms
            if (platform.type == 1) {
                // Moving platform - add direction indicators
                DrawCircle(
                    platform.rect.x + platform.rect.width * 0.2f,
                    platform.rect.y + platform.rect.height * 0.5f,
                    platform.rect.height * 0.15f,
                    (Color){50, 255, 50, 200}
                );
                DrawCircle(
                    platform.rect.x + platform.rect.width * 0.8f,
                    platform.rect.y + platform.rect.height * 0.5f,
                    platform.rect.height * 0.15f,
                    (Color){50, 255, 50, 200}
                );
            } else if (platform.type == 2) {
                // Breakable platform - add cracks
                DrawLineEx(
                    (Vector2){platform.rect.x + platform.rect.width * 0.3f, platform.rect.y},
                    (Vector2){platform.rect.x + platform.rect.width * 0.7f, platform.rect.y + platform.rect.height},
                    2.0f, (Color){50, 50, 50, 150}
                );
                DrawLineEx(
                    (Vector2){platform.rect.x + platform.rect.width * 0.7f, platform.rect.y},
                    (Vector2){platform.rect.x + platform.rect.width * 0.3f, platform.rect.y + platform.rect.height},
                    2.0f, (Color){50, 50, 50, 150}
                );
            }
        }
    }
    
    // Draw collectibles
    for (auto& collectible : collectibles) {
        if (!collectible.active) continue;
        
        switch (collectible.type) {
            case 0: // Coin
            {
                // Draw shiny coin with animation
                float pulse = (1.0f + sinf(GetTime() * 5.0f) * 0.2f);
                
                // Gold coin
                DrawCircle(
                    collectible.rect.x + collectible.rect.width * 0.5f,
                    collectible.rect.y + collectible.rect.height * 0.5f,
                    collectible.rect.width * 0.4f * pulse,
                    (Color){255, 215, 0, 255} // Gold
                );
                
                // Coin highlight
                DrawCircle(
                    collectible.rect.x + collectible.rect.width * 0.4f,
                    collectible.rect.y + collectible.rect.height * 0.4f,
                    collectible.rect.width * 0.15f * pulse,
                    (Color){255, 255, 200, 200}
                );
                
                // Coin border
                DrawCircleLines(
                    collectible.rect.x + collectible.rect.width * 0.5f,
                    collectible.rect.y + collectible.rect.height * 0.5f,
                    collectible.rect.width * 0.4f * pulse,
                    (Color){180, 150, 0, 255}
                );
                break;
            }
            
            case 1: // Health
            {
                // Health pack
                DrawRectangleRounded(
                    (Rectangle){
                        collectible.rect.x,
                        collectible.rect.y,
                        collectible.rect.width,
                        collectible.rect.height
                    },
                    0.3f, 8, (Color){230, 230, 230, 255} // White base
                );
                
                // Red cross
                DrawRectangle(
                    collectible.rect.x + collectible.rect.width * 0.4f,
                    collectible.rect.y + collectible.rect.height * 0.2f,
                    collectible.rect.width * 0.2f,
                    collectible.rect.height * 0.6f,
                    (Color){220, 40, 40, 255}
                );
                
                DrawRectangle(
                    collectible.rect.x + collectible.rect.width * 0.2f,
                    collectible.rect.y + collectible.rect.height * 0.4f,
                    collectible.rect.width * 0.6f,
                    collectible.rect.height * 0.2f,
                    (Color){220, 40, 40, 255}
                );
                break;
            }
            
            case 2: // Powerup
            {
                // Powerup (glowing orb)
                float pulse = (1.0f + sinf(GetTime() * 3.0f) * 0.3f);
                DrawCircleGradient(
                    collectible.rect.x + collectible.rect.width * 0.5f,
                    collectible.rect.y + collectible.rect.height * 0.5f,
                    collectible.rect.width * 0.4f * pulse,
                    (Color){100, 50, 200, 255}, // Purple core
                    (Color){180, 120, 255, 100} // Light purple glow
                );
                
                // Energy particles around powerup
                for (int i = 0; i < 6; i++) {
                    float angle = GetTime() * 3.0f + i * (PI * 2.0f / 6.0f);
                    float dist = collectible.rect.width * 0.3f;
                    float particleX = collectible.rect.x + collectible.rect.width * 0.5f + cosf(angle) * dist;
                    float particleY = collectible.rect.y + collectible.rect.height * 0.5f + sinf(angle) * dist;
                    
                    DrawCircle(
                        particleX, particleY,
                        collectible.rect.width * 0.1f,
                        (Color){200, 180, 255, 150}
                    );
                }
                break;
            }
        }
    }
    
    // Draw level exit portal
    if (levelExit.active) {
        // Draw swirling portal
        float time = GetTime() * 2.0f;
        float radius = levelExit.rect.width * 0.5f;
        Vector2 center = {
            levelExit.rect.x + levelExit.rect.width * 0.5f,
            levelExit.rect.y + levelExit.rect.height * 0.5f
        };
        
        // Portal outer glow
        DrawCircleGradient(
            center.x, center.y,
            radius * 1.5f,
            (Color){0, 200, 255, 100}, // Outer color (transparent cyan)
            (Color){0, 50, 150, 0}     // Fade to transparent
        );
        
        // Portal base
        DrawCircleGradient(
            center.x, center.y,
            radius,
            (Color){0, 150, 255, 255}, // Inner color (blue)
            (Color){0, 0, 150, 200}    // Outer color (dark blue)
        );
        
        // Draw spiral effect
        for (int i = 0; i < 4; i++) {
            float spiralAngle = time + i * (PI / 2.0f);
            for (float t = 0; t < radius; t += 2.0f) {
                float spiralX = center.x + cosf(spiralAngle + t * 0.5f) * t;
                float spiralY = center.y + sinf(spiralAngle + t * 0.5f) * t;
                
                DrawCircle(
                    spiralX, spiralY,
                    1.5f,
                    (Color){255, 255, 255, (unsigned char)(200 - t * 3)}
                );
            }
        }
        
        // Portal energy particles
        for (int i = 0; i < 8; i++) {
            float angle = time * 0.5f + i * (PI * 2.0f / 8.0f);
            float dist = radius * 0.6f * (0.7f + 0.3f * sinf(time * 3.0f + i));
            float particleX = center.x + cosf(angle) * dist;
            float particleY = center.y + sinf(angle) * dist;
            
            DrawCircle(
                particleX, particleY,
                3.0f,
                (Color){200, 255, 255, 200}
            );
        }
    }
    
    // Draw projectiles
    for (auto& proj : projectiles) {
        if (!proj.active) continue;
        
        if (proj.fromPlayer) {
            // Player projectile with energy trail
            DrawRectangleRounded(
                proj.rect,
                0.5f, 8, 
                (Color){50, 200, 255, 255} // Blue energy
            );
            
            // Energy trail
            for (int i = 1; i <= 5; i++) {
                float trailX = proj.rect.x - (proj.velocity.x > 0 ? 1 : -1) * i * 3.0f;
                float alpha = 200 - i * 40;
                if (alpha < 0) alpha = 0;
                
                DrawRectangleRounded(
                    (Rectangle){ 
                        trailX, 
                        proj.rect.y, 
                        proj.rect.width * (1.0f - i * 0.15f), 
                        proj.rect.height * (1.0f - i * 0.15f) 
                    },
                    0.5f, 8, 
                    (Color){50, 200, 255, (unsigned char)alpha}
                );
            }
        } else {
            // Enemy projectile (red energy)
            DrawRectangleRounded(
                proj.rect,
                0.5f, 8, 
                (Color){255, 50, 50, 255} // Red energy
            );
            
            // Energy core
            DrawRectangleRounded(
                (Rectangle){ 
                    proj.rect.x + proj.rect.width * 0.25f, 
                    proj.rect.y + proj.rect.height * 0.25f, 
                    proj.rect.width * 0.5f, 
                    proj.rect.height * 0.5f 
                },
                0.5f, 8, 
                (Color){255, 200, 200, 255}
            );
        }
    }
    
    // Draw enemies
    for (auto& enemy : enemies) {
        if (!enemy.active) continue;
        DrawDetailedEnemy(enemy);
    }
    
    // Draw player character with spacesuit and helmet
    float scale = 1.0f;
    Vector2 playerCenter = {
        player.rect.x + player.rect.width * 0.5f,
        player.rect.y + player.rect.height * 0.5f
    };
    DrawDetailedCharacter(playerCenter.x, playerCenter.y, scale, true); // Always with helmet in gameplay
    
    EndMode2D();
    
    // GUI overlay
    DrawRectangle(0, 0, GetScreenWidth(), 80, Fade((Color){20, 20, 50, 255}, 0.8f));
    
    // Player info
    DrawTextEx(customFont, TextFormat("Name: %s", playerName.c_str()), (Vector2){20, 10}, 20, 2, WHITE);
    DrawTextEx(customFont, TextFormat("Class: %s", fightingClasses[selectedFightingClass]), (Vector2){20, 40}, 20, 2, WHITE);
    
    // Health bar with decoration
    DrawRectangleRounded((Rectangle){200, 20, 200, 20}, 0.5f, 8, (Color){60, 60, 60, 255});
    DrawRectangleRounded(
        (Rectangle){200, 20, (player.health * 200 / playerMaxHealth), 20}, 
        0.5f, 8, 
        (Color){200, 50, 50, 255}
    );
    DrawTextEx(customFont, TextFormat("Health: %d/100", player.health), (Vector2){250, 20}, 20, 2, WHITE);
    
    // Energy bar
    DrawRectangleRounded((Rectangle){200, 45, 200, 15}, 0.5f, 8, (Color){60, 60, 60, 255});
    DrawRectangleRounded(
        (Rectangle){200, 45, (player.energy * 200 / playerMaxEnergy), 15}, 
        0.5f, 8, 
        (Color){50, 150, 255, 255}
    );
    DrawTextEx(customFont, TextFormat("Energy: %d/100", player.energy), (Vector2){250, 42}, 18, 2, WHITE);
    
    // Score and currency display
    DrawTextEx(customFont, TextFormat("Score: %d", player.score), (Vector2){500, 20}, 30, 2, YELLOW);
    
    // Currency with coin icon
    DrawCircle(500, 55, 10, GOLD);
    DrawCircleLines(500, 55, 10, (Color){180, 150, 0, 255});
    DrawTextEx(customFont, TextFormat("Credits: %d", player.currency), (Vector2){520, 50}, 25, 2, GOLD);
    
    // Level info
    DrawTextEx(customFont, TextFormat("Level: %d", currentLevel), (Vector2){(float)(GetScreenWidth() - 150), 20}, 30, 2, GREEN);
    DrawTextEx(customFont, TextFormat("Weapon: %s", weapons[selectedWeapon]), (Vector2){(float)(GetScreenWidth() - 350), 50}, 20, 2, WHITE);
    
    if (isPaused)
        DrawPauseMenu();
}

//------------------ Main Function ----------------------
int main() {
    InitWindow(screenWidth, screenHeight, "SPACE VENTURE v2.0");
    InitAudioDevice();
    SetTargetFPS(60);
    
    // Load font
    customFont = LoadFont("font/Overseer.otf");
    if (customFont.texture.id == 0) {
        TraceLog(LOG_WARNING, "Failed to load custom font! Using default font.");
    } else {
        TraceLog(LOG_INFO, "Custom font loaded successfully!");
    }
    
    // Load/initialize sounds
    backgroundMusic = LoadMusicStream("C:/raylib-5.5_win32_mingw-w64/Y&V - Lune  Electronic  NCS - Copyright Free Music.mp3");
    if (!IsMusicStreamPlaying(backgroundMusic)) {
        TraceLog(LOG_WARNING, "Failed to load or play background music!");
    } else { 
        TraceLog(LOG_INFO, "Background music loaded and playing successfully!"); 
        PlayMusicStream(backgroundMusic); 
    }
    
    // Load game sounds
    jumpSound = LoadSound("assets/jump.wav");
    shootSound = LoadSound("assets/shoot.wav");
    hitSound = LoadSound("assets/hit.wav");
    laserSound = LoadSound("assets/laser.wav");
    
    // Try to load additional sound effects
    coinSound = LoadSound("assets/coin.wav");
    portalSound = LoadSound("assets/portal.wav");
    levelCompleteSound = LoadSound("assets/level_complete.wav");
    
    // Main game loop
    while (!WindowShouldClose()) {
        UpdateMusicStream(backgroundMusic);
        
        switch(gameState) {
            case PLATFORMER:
                UpdatePlatformer();
                break;
            case SPACESHIP_COMBAT:
                // UpdateSpaceCombat(); -- Disabled until we implement it fully
                // For now, just return to main menu if space combat is selected
                gameState = MAIN_MENU;
                break;
            default:
                // Other states don't need continuous updates
                break;
        }
        
        BeginDrawing();
        
        switch(gameState) {
            case MAIN_MENU: 
                DrawMainMenu(); 
                break;
            case SETTINGS: 
                DrawSettingsMenu(); 
                break;
            case CHARACTER_CREATION: 
                DrawCharacterCreation(); 
                break;
            case CHARACTER_CUSTOMIZATION: 
                DrawCharacterCustomization(); 
                break;
            case PLAYING: 
                DrawPlaying(); 
                break;
            case PLATFORMER: 
                DrawPlatformer(); 
                break;
            case LEVEL_COMPLETE: 
                DrawLevelComplete(); 
                break;
            case SPACESHIP_COMBAT: 
                // DrawSpaceCombat(); -- Disabled until we implement it fully
                DrawMainMenu(); // Fallback
                gameState = MAIN_MENU;
                break;
        }
        
        EndDrawing();
    }
    
    // Unload font
    UnloadFont(customFont);
    
    // Unload music and sounds
    UnloadMusicStream(backgroundMusic);
    UnloadSound(jumpSound);
    UnloadSound(shootSound);
    UnloadSound(hitSound);
    UnloadSound(laserSound);
    UnloadSound(coinSound);
    UnloadSound(portalSound);
    UnloadSound(levelCompleteSound);
    
    CloseAudioDevice();
    CloseWindow();
    return 0;
}