#include <SDL2/SDL.h>
#include <iostream>
#include <vector>
#include <algorithm> // для std::min и std::max
#include <sstream>
#include <cctype>
#include <string>

class Coin {
public:
    float x, y;
    float width, height;
    bool isCollected;

    Coin(float posX, float posY) {
        x = posX;
        y = posY;
        width = 20;
        height = 20;
        isCollected = false;
    }

    SDL_FRect GetRect() const {
        return { x, y, width, height };
    }

    // Проверка коллизии с игроком
    bool CheckCollision(const SDL_FRect& playerRect) const {
        if (isCollected) return false;

        return (x < playerRect.x + playerRect.w &&
            x + width > playerRect.x &&
            y < playerRect.y + playerRect.h &&
            y + height > playerRect.y);
    }

    void Collect() {
        isCollected = true;
    }
};

class Player {
public:
    float x, y;          // Позиция
    float width, height; // Размер
    float velocityX, velocityY; // Скорость
    bool isOnGround;     // На земле ли
    bool canSprint;
    int lives;
    int coinsCollected;
    bool isAlive;
    float invincibilityTimer;

    float NORMAL_SPEED = 200.0f;
    float SPRINT_SPEED = 350.0f;
    float INVINCIBILITY_TIME = 2.0f;

    Player(float startX, float startY) {
        x = startX;
        y = startY;
        width = 50;
        height = 50;
        velocityX = 0;
        velocityY = 0;
        isOnGround = false;
        canSprint = true;
        coinsCollected = 0;
        lives = 3;
        isAlive = true;
        invincibilityTimer = 0.0f;
    }

    // Получить bounding box игрока
    SDL_FRect GetRect() const {
        return { x, y, width, height };
    }

    // Проверка коллизии с другим прямоугольником
    bool CheckCollision(const SDL_FRect& other) const {
        SDL_FRect rect = GetRect();
        return (rect.x < other.x + other.w &&
            rect.x + rect.w > other.x &&
            rect.y < other.y + other.h &&
            rect.y + rect.h > other.y);
    }

    // Разрешение коллизии
    void ResolveCollision(const SDL_FRect& platform) {
        SDL_FRect rect = GetRect();

        // Вычисляем глубину пересечения по каждой оси
        float overlapLeft = (rect.x + rect.w) - platform.x;    // Пересечение слева
        float overlapRight = (platform.x + platform.w) - rect.x; // Пересечение справа
        float overlapTop = (rect.y + rect.h) - platform.y;     // Пересечение сверху
        float overlapBottom = (platform.y + platform.h) - rect.y; // Пересечение снизу

        // Находим наименьшее пересечение
        float minOverlap = std::min({ overlapLeft, overlapRight, overlapTop, overlapBottom });

        // Разрешаем коллизию по наименьшему пересечению
        if (minOverlap == overlapTop && velocityY > 0) {
            // Коллизия сверху (игрок падает на платформу)
            y = platform.y - height;
            velocityY = 0;
            isOnGround = true;
        }
        else if (minOverlap == overlapBottom && velocityY < 0) {
            // Коллизия снизу (игрок ударяется головой)
            y = platform.y + platform.h;
            velocityY = 0;
        }
        else if (minOverlap == overlapLeft) {
            // Коллизия слева
            x = platform.x - width;
            velocityX = 0;
        }
        else if (minOverlap == overlapRight) {
            // Коллизия справа
            x = platform.x + platform.w;
            velocityX = 0;
        }
    }

    void Update(float deltaTime, const std::vector<SDL_FRect>& platforms) {
        // Обновляем таймер неуязвимости
        if (invincibilityTimer > 0) {
            invincibilityTimer -= deltaTime;
        }

        // Применяем гравитацию
        if (!isOnGround) {
            velocityY += 1000.0f * deltaTime; // Гравитация
        }
        
        canSprint = isOnGround;

        // Сохраняем старую позицию по Y для определения, были ли мы на земле
        float oldY = y;

        // Обновляем позицию по X
        x += velocityX * deltaTime;

        // Проверка коллизий по X
        SDL_FRect playerRect = GetRect();
        for (const auto& platform : platforms) {
            if (CheckCollision(platform)) {
                ResolveCollision(platform);
                break;
            }
        }

        // Обновляем позицию по Y
        y += velocityY * deltaTime;

        // Проверка коллизий по Y
        playerRect = GetRect();
        bool wasOnGround = isOnGround; // Сохраняем предыдущее состояние
        isOnGround = false; // Сбрасываем флаг земли

        for (const auto& platform : platforms) {
            if (CheckCollision(platform)) {
                ResolveCollision(platform);
            }
        }

        // Дополнительная проверка: если мы не на земле, но скорость близка к нулю и мы на платформе
        if (!isOnGround && std::abs(velocityY) < 1.0f) {
            SDL_FRect feetRect = { x, y + height - 1, width, 2 }; // Область под ногами
            for (const auto& platform : platforms) {
                if (feetRect.y < platform.y + platform.h &&
                    feetRect.y + feetRect.h > platform.y &&
                    feetRect.x < platform.x + platform.w &&
                    feetRect.x + feetRect.w > platform.x) {
                    isOnGround = true;
                    break;
                }
            }
        }
        // Проверка границ экрана (смерть при падении)
        if (y > 600) {
            TakeDamage();
        }
        if (x < 0) x = 0;
        if (x > 800 - width) x = 800 - width; 


    }
    // Метод для получения урона
    void TakeDamage() {
        if (invincibilityTimer > 0 || !isAlive) return;

        lives--;
        invincibilityTimer = INVINCIBILITY_TIME;

        std::cout << "Player hit! Lives: " << lives << std::endl;

        if (lives <= 0) {
            isAlive = false;
            std::cout << "Game Over!" << std::endl;
        }
        else {
            // Респавн после получения урона
            x = 100;
            y = 100;
            velocityX = 0;
            velocityY = 0;
        }
    }

    // Метод для проверки неуязвимости (для мигания)
    bool IsInvincible() const {
        return invincibilityTimer > 0;
    }

    void Jump() {
        if (isOnGround) {
            velocityY = -500.0f; // Сила прыжка
            isOnGround = false;
        }
    }

    void MoveLeft(bool isSprinting = false) {
        if (isSprinting && canSprint) {
            velocityX = -SPRINT_SPEED;
        }
        else {
            velocityX = -NORMAL_SPEED;
        }
    }

    void MoveRight(bool isSprinting = false) {
        if (isSprinting && canSprint) {
            velocityX = SPRINT_SPEED;
        }
        else {
            velocityX = NORMAL_SPEED;
        }
    }

    void Stop() {
        velocityX = 0;
    }

    void CollectCoin() {
        coinsCollected++;
        std::cout << "Coin collected! Total: " << coinsCollected << std::endl;
    }
};

class Enemy {
public:
    float x, y;
    float width, height;
    float velocityX;
    float patrolDistance;
    float startX;
    bool isActive;

    Enemy(float posX, float posY, float patrolDist = 100.0f) {
        x = posX;
        y = posY;
        width = 40;
        height = 40;
        velocityX = 50.0f;
        patrolDistance = patrolDist;
        startX = posX;
        isActive = true;
    }

    SDL_FRect GetRect() const {
        return { x, y, width, height };
    }

    void Update(float deltaTime) {
        if (!isActive) return;

        // Движение вперед-назад
        x += velocityX * deltaTime;

        // Разворот при достижении границ патрулирования
        if (x > startX + patrolDistance) {
            velocityX = -abs(velocityX);
        }
        else if (x < startX - patrolDistance) {
            velocityX = abs(velocityX);
        }
    }

    // Проверка коллизии с игроком
    bool CheckCollision(const SDL_FRect& playerRect) const {
        if (!isActive) return false;

        return (x < playerRect.x + playerRect.w &&
            x + width > playerRect.x &&
            y < playerRect.y + playerRect.h &&
            y + height > playerRect.y);
    }
};

int main(int argc, char* argv[]) {
    std::cout << "Starting Platformer Game with SDL2..." << std::endl;

    // Инициализация SDL
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr << "SDL_Init Error: " << SDL_GetError() << std::endl;
        return 1;
    }

    std::cout << "SDL2 initialized successfully!" << std::endl;

    // Создаем окно SDL
    SDL_Window* window = SDL_CreateWindow("Platformer Game - SDL2 - Press ESC to exit",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        800, 600, SDL_WINDOW_SHOWN);

    if (window == nullptr) {
        std::cerr << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    // Создаем рендерер
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    if (renderer == nullptr) {
        std::cerr << "SDL_CreateRenderer Error: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Создаем игрока
    Player player(100, 100);

    std::vector<Coin> coins = {
    Coin(250, 350),  // Монетка над первой платформой
    Coin(150, 250),  // Монетка над второй платформой  
    Coin(550, 200),  // Монетка над третьей платформой
    Coin(75, 450),   // Дополнительная монетка
    Coin(675, 400)   // Еще одна монетка
    };

    std::vector<Enemy> enemies = {
    Enemy(300, 350, 150),  // Враг на основной платформе
    Enemy(150, 250, 80),   // Враг на верхней левой платформе
    Enemy(550, 200, 100),  // Враг на верхней правой платформе
    Enemy(50, 450, 50)     // Враг на маленькой платформе
    };

    SDL_Rect camera = { 0, 0, 800, 600 };

    // Создаем платформы (x, y, width, height)
    std::vector<SDL_FRect> platforms = {
        {200.0f, 400.0f, 400.0f, 20.0f},  // Основная платформа
        {100.0f, 300.0f, 200.0f, 20.0f},  // Верхняя левая
        {500.0f, 250.0f, 200.0f, 20.0f},  // Верхняя правая
        {0.0f, 580.0f, 800.0f, 20.0f},    // Земля
        {50.0f, 500.0f, 100.0f, 20.0f},   // Маленькая платформа
        {650.0f, 450.0f, 100.0f, 20.0f}   // Еще платформа
    };

    std::cout << "Game started! Use A/D to move, SPACE to jump, ESC to exit." << std::endl;

    // Главный игровой цикл
    bool running = true;
    int frameCount = 0;
    Uint32 lastTime = SDL_GetTicks();
    const Uint8* keyboardState = SDL_GetKeyboardState(NULL);

    // Простая функция для рисования символов
    auto DrawSimpleChar = [](SDL_Renderer* renderer, char c, int x, int y) {
        // Приводим к верхнему регистру для упрощения
        c = std::toupper(c);

        switch (c) {
            // Цифры (оставляем как было)
        case '0': {
            SDL_Rect segments[] = {
                {x, y, 8, 2}, {x, y + 2, 2, 8}, {x + 6, y + 2, 2, 8}, {x, y + 10, 8, 2}
            };
            for (int i = 0; i < 4; i++) SDL_RenderFillRect(renderer, &segments[i]);
            break;
        }
        case '1': {
            SDL_Rect seg = { x + 3, y, 2, 12 };
            SDL_RenderFillRect(renderer, &seg);
            break;
        }
        case '2': {
            SDL_Rect segments[] = {
                {x, y, 8, 2}, {x + 6, y + 2, 2, 3}, {x, y + 5, 8, 2},
                {x, y + 7, 2, 3}, {x, y + 10, 8, 2}
            };
            for (int i = 0; i < 5; i++) SDL_RenderFillRect(renderer, &segments[i]);
            break;
        }
        case '3': {
            SDL_Rect segments[] = {
                {x, y, 8, 2}, {x + 6, y + 2, 2, 3}, {x, y + 5, 8, 2},
                {x + 6, y + 7, 2, 3}, {x, y + 10, 8, 2}
            };
            for (int i = 0; i < 5; i++) SDL_RenderFillRect(renderer, &segments[i]);
            break;
        }
        case '4': {
            SDL_Rect segments[] = {
                {x, y, 2, 5}, {x, y + 5, 8, 2}, {x + 6, y, 2, 12}
            };
            for (int i = 0; i < 3; i++) SDL_RenderFillRect(renderer, &segments[i]);
            break;
        }
        case '5': {
            SDL_Rect segments[] = {
                {x, y, 8, 2}, {x, y + 2, 2, 3}, {x, y + 5, 8, 2},
                {x + 6, y + 7, 2, 3}, {x, y + 10, 8, 2}
            };
            for (int i = 0; i < 5; i++) SDL_RenderFillRect(renderer, &segments[i]);
            break;
        }
        case '6': {
            SDL_Rect segments[] = {
                {x, y, 8, 2}, {x, y + 2, 2, 8}, {x, y + 5, 8, 2},
                {x + 6, y + 7, 2, 3}, {x, y + 10, 8, 2}
            };
            for (int i = 0; i < 5; i++) SDL_RenderFillRect(renderer, &segments[i]);
            break;
        }
        case '7': {
            SDL_Rect segments[] = {
                {x, y, 8, 2}, {x + 6, y + 2, 2, 10}
            };
            for (int i = 0; i < 2; i++) SDL_RenderFillRect(renderer, &segments[i]);
            break;
        }
        case '8': {
            SDL_Rect segments[] = {
                {x, y, 8, 2}, {x, y + 2, 2, 8}, {x + 6, y + 2, 2, 8},
                {x, y + 5, 8, 2}, {x, y + 10, 8, 2}
            };
            for (int i = 0; i < 5; i++) SDL_RenderFillRect(renderer, &segments[i]);
            break;
        }
        case '9': {
            SDL_Rect segments[] = {
                {x, y, 8, 2}, {x, y + 2, 2, 3}, {x + 6, y + 2, 2, 8},
                {x, y + 5, 8, 2}, {x, y + 10, 8, 2}
            };
            for (int i = 0; i < 5; i++) SDL_RenderFillRect(renderer, &segments[i]);
            break;
        }
        case '/': {
            for (int i = 0; i < 4; i++) {
                SDL_Rect seg = { x + i, y + 2 + i * 2, 2, 2 };
                SDL_RenderFillRect(renderer, &seg);
            }
            break;
        }

                // БУКВЫ для "LIVES:"
        case 'L': {
            SDL_Rect segments[] = {
                {x, y, 2, 10}, {x, y + 8, 6, 2}
            };
            for (int i = 0; i < 2; i++) SDL_RenderFillRect(renderer, &segments[i]);
            break;
        }
        case 'I': {
            SDL_Rect segments[] = {
                {x, y, 8, 2}, {x + 3, y + 2, 2, 8}, {x, y + 10, 8, 2}
            };
            for (int i = 0; i < 3; i++) SDL_RenderFillRect(renderer, &segments[i]);
            break;
        }
        case 'V': {
            SDL_Rect segments[] = {
                {x, y, 2, 8}, {x + 6, y, 2, 8}, {x + 2, y + 8, 4, 2}, {x + 1, y + 10, 6, 2}
            };
            for (int i = 0; i < 4; i++) SDL_RenderFillRect(renderer, &segments[i]);
            break;
        }
        case 'E': {
            SDL_Rect segments[] = {
                {x, y, 8, 2}, {x, y + 2, 2, 8}, {x, y + 5, 8, 2}, {x, y + 10, 8, 2}
            };
            for (int i = 0; i < 4; i++) SDL_RenderFillRect(renderer, &segments[i]);
            break;
        }
        case 'S': {
            SDL_Rect segments[] = {
                {x, y, 8, 2}, {x, y + 2, 2, 3}, {x, y + 5, 8, 2},
                {x + 6, y + 7, 2, 3}, {x, y + 10, 8, 2}
            };
            for (int i = 0; i < 5; i++) SDL_RenderFillRect(renderer, &segments[i]);
            break;
        }
        case ':': {
            SDL_Rect segments[] = {
                {x + 3, y + 3, 2, 2}, {x + 3, y + 7, 2, 2}
            };
            for (int i = 0; i < 2; i++) SDL_RenderFillRect(renderer, &segments[i]);
            break;
        }

                // БУКВЫ для "GAME OVER" и "PRESS R TO RESTART"
        case 'G': {
            SDL_Rect segments[] = {
                {x, y, 8, 2}, {x, y + 2, 2, 8}, {x, y + 10, 8, 2},
                {x + 6, y + 6, 2, 4}, {x + 4, y + 6, 2, 2}
            };
            for (int i = 0; i < 5; i++) SDL_RenderFillRect(renderer, &segments[i]);
            break;
        }
        case 'A': {
            SDL_Rect segments[] = {
                {x, y, 8, 2}, {x, y + 2, 2, 8}, {x + 6, y + 2, 2, 8},
                {x, y + 5, 8, 2}
            };
            for (int i = 0; i < 4; i++) SDL_RenderFillRect(renderer, &segments[i]);
            break;
        }
        case 'M': {
            SDL_Rect segments[] = {
                {x, y, 2, 10}, {x + 6, y, 2, 10}, {x + 2, y + 2, 1, 2},
                {x + 3, y + 3, 2, 2}, {x + 5, y + 2, 1, 2}
            };
            for (int i = 0; i < 5; i++) SDL_RenderFillRect(renderer, &segments[i]);
            break;
        }
        case 'O': {
            SDL_Rect segments[] = {
                {x, y, 8, 2}, {x, y + 2, 2, 8}, {x + 6, y + 2, 2, 8}, {x, y + 10, 8, 2}
            };
            for (int i = 0; i < 4; i++) SDL_RenderFillRect(renderer, &segments[i]);
            break;
        }
        case 'R': {
            SDL_Rect segments[] = {
                {x, y, 2, 10}, {x, y, 8, 2}, {x + 6, y + 2, 2, 3},
                {x, y + 5, 8, 2}, {x + 4, y + 7, 2, 5}
            };
            for (int i = 0; i < 5; i++) SDL_RenderFillRect(renderer, &segments[i]);
            break;
        }
        case 'P': {
            SDL_Rect segments[] = {
                {x, y, 2, 10}, {x, y, 8, 2}, {x + 6, y + 2, 2, 3}, {x, y + 5, 8, 2}
            };
            for (int i = 0; i < 4; i++) SDL_RenderFillRect(renderer, &segments[i]);
            break;
        }
        case 'T': {
            SDL_Rect segments[] = {
                {x, y, 8, 2}, {x + 3, y + 2, 2, 8}
            };
            for (int i = 0; i < 2; i++) SDL_RenderFillRect(renderer, &segments[i]);
            break;
        }
        case 'U': {
            SDL_Rect segments[] = {
                {x, y, 2, 10}, {x + 6, y, 2, 10}, {x, y + 10, 8, 2}
            };
            for (int i = 0; i < 3; i++) SDL_RenderFillRect(renderer, &segments[i]);
            break;
        }
        case ' ': {
            // Пробел - ничего не рисуем
            break;
        }
        default:
            // Для неизвестных символов рисуем простой прямоугольник
            SDL_Rect unknown = { x, y, 6, 10 };
            SDL_RenderFillRect(renderer, &unknown);
            break;
        }

        };

    while (running) {
        Uint32 currentTime = SDL_GetTicks();
        float deltaTime = (currentTime - lastTime) / 1000.0f;
        lastTime = currentTime;



        // Game Over проверка ДО ВСЕГО остального
        if (!player.isAlive) {
            

            // Game Over экран
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderClear(renderer);

            SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
            std::string gameOverText = "GAME OVER";
            int textX = 250;

            
            for (char c : gameOverText) {
                
                DrawSimpleChar(renderer, c, textX, 280);
                textX += 10;
            }

            std::string restartText = "PRESS R TO RESTART";
            textX = 200;

            
            for (char c : restartText) {                
                DrawSimpleChar(renderer, c, textX, 320);
                textX += 8;
            }

            

            SDL_RenderPresent(renderer);
            

            // Обработка рестарта
            SDL_Event event;
            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_QUIT ||
                    (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE)) {
                    running = false;
                }
                if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_r) {
                    // Рестарт игры
                    player = Player(100, 100);
                    for (auto& coin : coins) {
                        coin.isCollected = false;
                    }
                    for (auto& enemy : enemies) {
                        enemy.isActive = true;
                        enemy.x = enemy.startX;
                    }
                }
            }

            SDL_Delay(16);
            continue;
        }



        static int coinDisplayCounter = 0;
        coinDisplayCounter++;
        if (coinDisplayCounter % 60 == 0) {
            std::cout << "Coins: " << player.coinsCollected << "/" << coins.size()
                << " | Lives: " << player.lives
                << " | Invincible: " << (player.IsInvincible() ? "Yes" : "No") << std::endl;
        }

        // Обработка событий
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT ||
                (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE)) {
                running = false;
            }
            if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_SPACE && player.isAlive) {
                player.Jump();
            }
        }

        // Обработка непрерывного ввода (только если игрок жив)
        if (player.isAlive) {
            bool isSprinting = keyboardState[SDL_SCANCODE_LSHIFT];

            if (keyboardState[SDL_SCANCODE_A]) {
                player.MoveLeft(isSprinting);
            }
            else if (keyboardState[SDL_SCANCODE_D]) {
                player.MoveRight(isSprinting);
            }
            else {
                player.Stop();
            }
        }

        // Обновление игрока с коллизиями
        player.Update(deltaTime, platforms);

        // Проверка сбора монет (ОДИН РАЗ объявляем playerRect)
        SDL_FRect playerRect = player.GetRect();
        for (auto& coin : coins) {
            if (coin.CheckCollision(playerRect)) {
                coin.Collect();
                player.CollectCoin();
            }
        } 

        // Обновление врагов
        for (auto& enemy : enemies) {
            enemy.Update(deltaTime);
        }

        // Проверка столкновений с врагами
        if (player.isAlive && !player.IsInvincible()) {
            SDL_FRect playerRect = player.GetRect();
            for (auto& enemy : enemies) {
                if (enemy.CheckCollision(playerRect)) {
                    player.TakeDamage();
                    break; // Чтобы не получать урон от нескольких врагов сразу
                }
            }
        }

        // Обновляем камеру (следим за игроком)
        camera.x = static_cast<int>(player.x + player.width / 2 - 400);
        camera.y = static_cast<int>(player.y + player.height / 2 - 300);

        // Ограничиваем камеру границами уровня
        if (camera.x < 0) camera.x = 0;
        if (camera.y < 0) camera.y = 0;
        if (camera.x > 1600 - camera.w) camera.x = 1600 - camera.w;
        if (camera.y > 1200 - camera.h) camera.y = 1200 - camera.h;

        // Очистка экрана
        SDL_SetRenderDrawColor(renderer, 68, 51, 85, 255);
        SDL_RenderClear(renderer);

        // Рисуем платформы
        SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
        for (const auto& platform : platforms) {
            SDL_FRect platformScreenRect = {
                platform.x - camera.x,
                platform.y - camera.y,
                platform.w,
                platform.h
            };
            SDL_RenderFillRectF(renderer, &platformScreenRect);
        }

        // Рисуем монетки (ОТДЕЛЬНЫМ ЦИКЛОМ)
        SDL_SetRenderDrawColor(renderer, 255, 215, 0, 255);
        for (const auto& coin : coins) {
            if (!coin.isCollected) {
                SDL_FRect coinScreenRect = {
                    coin.x - camera.x,
                    coin.y - camera.y,
                    coin.width,
                    coin.height
                };
                SDL_RenderFillRectF(renderer, &coinScreenRect);
            }
        }

        // Рисуем врагов (красные с черными глазами)
        for (const auto& enemy : enemies) {
            if (!enemy.isActive) continue;

            SDL_FRect enemyScreenRect = {
                enemy.x - camera.x,
                enemy.y - camera.y,
                enemy.width,
                enemy.height
            };

            // Тело врага
            SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
            SDL_RenderFillRectF(renderer, &enemyScreenRect);

            // Глаза врага
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_FRect leftEye = { enemy.x - camera.x + 8, enemy.y - camera.y + 10, 8, 8 };
            SDL_FRect rightEye = { enemy.x - camera.x + 24, enemy.y - camera.y + 10, 8, 8 };
            SDL_RenderFillRectF(renderer, &leftEye);
            SDL_RenderFillRectF(renderer, &rightEye);
        }

        // Рисуем игрока (используем существующий playerRect)
        SDL_FRect playerScreenRect = {
            playerRect.x - camera.x,
            playerRect.y - camera.y,
            playerRect.w,
            playerRect.h
            };
        // Мигание при неуязвимости
        if (!player.IsInvincible() || (static_cast<int>(player.invincibilityTimer * 10) % 2 == 0)) {
            SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
            SDL_RenderFillRectF(renderer, &playerScreenRect);
        }


        // Рисуем панель счета в левом верхнем углу
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 128);
        SDL_Rect scoreBackground = { 10, 10, 150, 40 };
        SDL_RenderFillRect(renderer, &scoreBackground);

        // Рисуем иконку монетки
        SDL_SetRenderDrawColor(renderer, 255, 215, 0, 255);
        SDL_Rect coinIcon = { 20, 20, 15, 15 };
        SDL_RenderFillRect(renderer, &coinIcon);

        // Рисуем текст счета монет
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        char scoreText[50];
        sprintf_s(scoreText, "%d / %zu", player.coinsCollected, coins.size());





        // Рисуем текст
        int textX = 40;
        for (int i = 0; scoreText[i] != '\0'; i++) {
            DrawSimpleChar(renderer, scoreText[i], textX, 22);
            textX += (scoreText[i] == ' ') ? 6 : 10; // Меньше места для пробелов
        }

        // Рисуем текст "LIVES:"
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        std::string livesText = "LIVES:";
        int livesTextX = 20;
        for (char c : livesText) {
            DrawSimpleChar(renderer, c, livesTextX, 55);
            livesTextX += 10;
        }

        // Рисуем сердца для жизней
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        for (int i = 0; i < player.lives; i++) {
            // Простое сердце - красный квадрат
            SDL_Rect heart = { 70 + i * 25, 55, 20, 20 };
            SDL_RenderFillRect(renderer, &heart);
        }

        // Обновляем экран
        SDL_RenderPresent(renderer);



        SDL_Delay(16);
    }

            // Очистка ресурсов
            SDL_DestroyRenderer(renderer);
            SDL_DestroyWindow(window);
            SDL_Quit();

            std::cout << "Game finished successfully!" << std::endl;
            return 0;
        }
    