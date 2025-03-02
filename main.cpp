#include <SFML/Graphics.hpp>
#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>

const int windowWidth = 800;
const int windowHeight = 600;
const float blockHeight = 30.0f;
const float initialBlockWidth = 200.0f;
float blockSpeed = 3.0f;
float cameraOffset = 0.0f;
int score = 0;
int perfectCombo = 0;
const int maxBlocksBeforeCameraMove = 6;

void resetGame(std::vector<sf::RectangleShape>& blocks, sf::RectangleShape& movingBlock, bool& blockPlaced, bool& gameOver);

sf::RectangleShape createStaticBlock(float yPosition) {
    sf::RectangleShape block(sf::Vector2f(200.0f, blockHeight));
    block.setFillColor(sf::Color::White);
    block.setPosition(windowWidth / 2 - block.getSize().x / 2, yPosition);
    return block;
}

sf::RectangleShape createDynamicBlock(float yPosition, bool spawnOnLeft, float dynamicWidth) {
    sf::RectangleShape block(sf::Vector2f(dynamicWidth, blockHeight));
    block.setFillColor(sf::Color::White);
    
    if (spawnOnLeft) {
        block.setPosition(0, yPosition);
    } else {
        block.setPosition(windowWidth - block.getSize().x, yPosition);
    }
    
    return block;
}

void updateBlockPosition(sf::RectangleShape& block, bool& movingRight) {
    block.move(movingRight ? blockSpeed : -blockSpeed, 0);
    if (block.getPosition().x <= 0)
        movingRight = true;
    else if (block.getPosition().x + block.getSize().x >= windowWidth)
        movingRight = false;
}

float getOverlapWidth(const sf::RectangleShape& block1, const sf::RectangleShape& block2) {
    float left = std::max(block1.getPosition().x, block2.getPosition().x);
    float right = std::min(block1.getPosition().x + block1.getSize().x, block2.getPosition().x + block2.getSize().x);
    return right - left;
}

void handleClick(sf::RectangleShape& movingBlock, std::vector<sf::RectangleShape>& blocks, bool& blockPlaced, bool& gameOver, bool& movingRight) {
    if (!blockPlaced) {
        float overlapWidth = getOverlapWidth(movingBlock, blocks.back());

        if (overlapWidth <= 0) {
            gameOver = true;
            return;
        }

        float newBlockWidth = overlapWidth;
        float newX = std::max(movingBlock.getPosition().x, blocks.back().getPosition().x);

        movingBlock.setPosition(newX, blocks.back().getPosition().y - blockHeight);

        if (newBlockWidth == blocks.back().getSize().x) {
            std::cout << "Coup Parfait" << std::endl;
            perfectCombo++;

            if (perfectCombo >= 2) {
                if (newBlockWidth < initialBlockWidth) {
                    newBlockWidth = initialBlockWidth;
                } else {
                    score += 2;
                }
            }
        } else {
            perfectCombo = 0;
        }

        sf::RectangleShape newBlock(sf::Vector2f(newBlockWidth, blockHeight));
        newBlock.setFillColor(sf::Color::White);
        newBlock.setPosition(newX, movingBlock.getPosition().y);

        blocks.push_back(newBlock);

        movingBlock = createDynamicBlock(blocks.back().getPosition().y - blockHeight, rand() % 2 == 0, newBlockWidth);

        score++;
        blockSpeed += 0.2f;
        blockPlaced = true;
    }
}

void resetGame(std::vector<sf::RectangleShape>& blocks, sf::RectangleShape& movingBlock, bool& blockPlaced, bool& gameOver) {
    blockSpeed = 3.0f;
    score = 0;
    cameraOffset = 0.0f;
    blocks.clear();
    blocks.push_back(createStaticBlock(windowHeight - 1 * blockHeight));
    movingBlock = createDynamicBlock(windowHeight - 2 * blockHeight, rand() % 2 == 0, 200.0f);
    blockPlaced = false;
    gameOver = false;
}

int main() {
    sf::RenderWindow window(sf::VideoMode(windowWidth, windowHeight), "Stacks 2D");
    window.setFramerateLimit(60);

    srand(static_cast<unsigned int>(time(0)));

    bool movingRight = true;
    bool blockPlaced = false;
    bool gameOver = false;

    sf::RectangleShape staticBlock = createStaticBlock(windowHeight - 1 * blockHeight);
    sf::RectangleShape movingBlock = createDynamicBlock(windowHeight - 2 * blockHeight, rand() % 2 == 0, 200.0f);

    std::vector<sf::RectangleShape> blocks;
    blocks.push_back(staticBlock);

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();

            if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
                handleClick(movingBlock, blocks, blockPlaced, gameOver, movingRight);
            }
        }

        if (!blockPlaced) {
            updateBlockPosition(movingBlock, movingRight);
        }

        if (score >= maxBlocksBeforeCameraMove) {
            cameraOffset = (score - maxBlocksBeforeCameraMove) * blockHeight;
        }

        window.clear();

        for (auto block : blocks) {
            sf::RectangleShape tempBlock = block;
            tempBlock.move(0, +cameraOffset);
            window.draw(tempBlock);
        }

        if (!blockPlaced) {
            sf::RectangleShape tempMovingBlock = movingBlock;
            tempMovingBlock.move(0, +cameraOffset);
            window.draw(tempMovingBlock);
        }

        sf::Font font;
        if (font.loadFromFile("assets/fonts/Arial.ttf")) {
            sf::Text scoreText;
            scoreText.setFont(font);
            scoreText.setString("Score: " + std::to_string(score));
            scoreText.setCharacterSize(30);
            scoreText.setFillColor(sf::Color::White);
            scoreText.setPosition(10, 10);
            window.draw(scoreText);
        }

        if (gameOver) {
            resetGame(blocks, movingBlock, blockPlaced, gameOver);
        }

        window.display();

        if (blockPlaced) {
            blockPlaced = false;
        }
    }

    return 0;
}
