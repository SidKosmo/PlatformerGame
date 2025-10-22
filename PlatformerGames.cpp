#include <SDL2/SDL.h>
#include <iostream>
#include <vector>
#include <algorithm> // для std::min и std::max

// Класс игрока
class Player {
public:
    float x, y;          // Позиция
    float width, height; // Размер
    float velocityX, velocityY; // Скорость
    bool isOnGround;     // На земле ли
    bool canSprint;

    const float NORMAL_SPEED = 200.0f;
    const float SPRINT_SPEED = 350.0f;

    Player(float startX, float startY) {
        x = startX;
        y = startY;
        width = 50;
        height = 50;
        velocityX = 0;
        velocityY = 0;
        isOnGround = false;
        canSprint = true;
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

        // Проверка границ экрана
        if (x < 0) x = 0;
        if (x > 800 - width) x = 800 - width;
        if (y > 600) {
            y = 100; // Респавн при падении
            x = 100;
            velocityY = 0;
            isOnGround = false;
        }
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

    while (running) {
        Uint32 currentTime = SDL_GetTicks();
        float deltaTime = (currentTime - lastTime) / 1000.0f;
        lastTime = currentTime;

        // Обработка событий
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT ||
                (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE)) {
                running = false;
            }
            if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_SPACE) {
                player.Jump();
            }
        }

        // Обработка непрерывного ввода
        if (keyboardState[SDL_SCANCODE_A]) player.MoveLeft();
        else if (keyboardState[SDL_SCANCODE_D]) player.MoveRight();
        else player.Stop();

        // Обновление игрока с коллизиями
        player.Update(deltaTime, platforms);

        // Обновляем камеру (следим за игроком)
        camera.x = static_cast<int>(player.x + player.width / 2 - 400);
        camera.y = static_cast<int>(player.y + player.height / 2 - 300);

        // Ограничиваем камеру границами уровня
        if (camera.x < 0) camera.x = 0;
        if (camera.y < 0) camera.y = 0;
        if (camera.x > 1600 - camera.w) camera.x = 1600 - camera.w; // если уровень шире 1600
        if (camera.y > 1200 - camera.h) camera.y = 1200 - camera.h; // если уровень выше 1200

        // Очистка экрана
        SDL_SetRenderDrawColor(renderer, 68, 51, 85, 255);
        SDL_RenderClear(renderer);

        // Рисуем игрока (с учетом камеры)
        SDL_FRect playerRect = player.GetRect();
        SDL_FRect playerScreenRect = {
            playerRect.x - camera.x,
            playerRect.y - camera.y,
            playerRect.w,
            playerRect.h
        };
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        SDL_RenderFillRectF(renderer, &playerScreenRect);

        // Рисуем платформы (с учетом камеры)
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

        // Обновляем экран
        SDL_RenderPresent(renderer);

        // FPS счетчик
        frameCount++;
        if (currentTime - lastTime >= 1000) {
            std::cout << "FPS: " << frameCount << std::endl;
            frameCount = 0;
            lastTime = currentTime;
        }

        SDL_Delay(16);
    }

    // Очистка ресурсов
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    std::cout << "Game finished successfully!" << std::endl;
    return 0;
}