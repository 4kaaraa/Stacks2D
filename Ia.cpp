#include <SFML/Graphics.hpp>
#include <fann.h>
#include <fann_cpp.h>
#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>

const int windowWidth = 800;
const int windowHeight = 600;
const float blockHeight = 30.0f;
const float initialBlockWidth = 200.0f;
float totalReward = 0.0f;
float lastReward = 0.0f;
float blockSpeed = 25.0f;
float cameraOffset = 0.0f;
int score = 0;
int perfectCombo = 0;
const int maxBlocksBeforeCameraMove = 6;
const int timeLimit = 5;

struct AI {
    FANN::neural_net net;

    AI() {
        net.create_standard(3, 2, 3, 1);
        net.set_learning_rate(0.7);
        net.set_activation_function_hidden(FANN::SIGMOID_SYMMETRIC);
        net.set_activation_function_output(FANN::SIGMOID_SYMMETRIC);
    }

    int makeDecision(float overlap, float speed, float blockPosition) {
        fann_type input[3] = { overlap, speed, blockPosition };
        fann_type* output = net.run(input);
        return (output[0] > 0.5) ? 1 : 0;
    }

    void train(float overlap, float speed, float blockPosition, float reward) {
        if (reward != 0) {
            fann_type input[3] = { overlap, speed, blockPosition };
            fann_type output[1] = { reward };
            net.train(input, output);
        }
    }

    void saveModel() {
        net.save("ai_model.net");
    }

    void loadModel() {
        net.create_from_file("ai_model.net");
        std::cout << "Loading" << std::endl;
    }
};

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

float calculateDistanceFromCenter(float blockPosition) {
    float center = windowWidth / 2;
    return std::abs(center - blockPosition);
}

void handleClick(sf::RectangleShape& movingBlock, std::vector<sf::RectangleShape>& blocks, bool& blockPlaced, bool& gameOver, AI& ai, sf::Clock& blockTimer) {
    if (!blockPlaced) {
        float overlapWidth = getOverlapWidth(movingBlock, blocks.back());

        if (overlapWidth <= 0) {
            gameOver = true;
            ai.train(0, blockSpeed, movingBlock.getPosition().x, -1000);
            return;
        }

        float newBlockWidth = overlapWidth;
        float newX = std::max(movingBlock.getPosition().x, blocks.back().getPosition().x);
        movingBlock.setPosition(newX, blocks.back().getPosition().y - blockHeight);

        float reward = 0.0f;
        
        // Calculer la récompense en fonction de l'overlap et de la position du bloc
        float distanceFromCenter = calculateDistanceFromCenter(movingBlock.getPosition().x);
        reward += (1.0f / (1.0f + distanceFromCenter / 100.0f));  // Plus c'est loin du centre, plus c'est récompensé

        if (overlapWidth >= blocks.back().getSize().x * 0.9 && newBlockWidth >= 150.0f) {
            std::cout << "[AAAAAA] Bon coup: " << score << std::endl;
            score++;
            perfectCombo++;
            reward += 100.0f;
            totalReward += reward;

            if (perfectCombo == 5) {
                std::cout << "[++++] Combo: " << score << std::endl;
                reward += 1000.0f;
                totalReward += reward;
                score += 10;
            }

            std::cout << "Reward: " << reward << std::endl;
        } else if (overlapWidth >= blocks.back().getSize().x * 0.75 && newBlockWidth >= 75.0f) {
            perfectCombo = 0;
            reward += 0.5f;
            totalReward += reward;
            std::cout << "Reward: " << reward << std::endl;
        } else if (overlapWidth >= blocks.back().getSize().x * 0.5 && newBlockWidth >= 50.0f) {
            perfectCombo = 0;
            reward += 0.3f;
            totalReward += reward;
            std::cout << "Reward: " << reward << std::endl;
        } else if (overlapWidth >= blocks.back().getSize().x * 0.3 && newBlockWidth >= 25.0f) {
            perfectCombo = 0;
            reward += 0.1f;
            totalReward += reward;
            std::cout << "Reward: " << reward << std::endl;
        } else {
            perfectCombo = 0;
            reward -= 0.2f;
            totalReward += reward;
            std::cout << "Reward: " << reward << std::endl;
        }

        ai.train(overlapWidth, blockSpeed, movingBlock.getPosition().x, reward);

        sf::RectangleShape newBlock(sf::Vector2f(newBlockWidth, blockHeight));
        newBlock.setFillColor(sf::Color::White);
        newBlock.setPosition(newX, movingBlock.getPosition().y);
        blocks.push_back(newBlock);

        movingBlock = createDynamicBlock(blocks.back().getPosition().y - blockHeight, rand() % 2 == 0, newBlockWidth);
        score++;
        blockSpeed += 0.2f;
        blockPlaced = false;
        blockTimer.restart();
    }
}

void resetGame(std::vector<sf::RectangleShape>& blocks, sf::RectangleShape& movingBlock, bool& blockPlaced, bool& gameOver) {
    blockSpeed = 25.0f;
    totalReward = 0.0f;
    score = 0;
    cameraOffset = 0.0f;
    blocks.clear();
    blocks.push_back(createStaticBlock(windowHeight - 1 * blockHeight));
    movingBlock = createDynamicBlock(windowHeight - 2 * blockHeight, rand() % 2 == 0, 200.0f);
    blockPlaced = false;
    gameOver = false;
}

int main() {
    try {
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

        AI ai;
        ai.loadModel();
        sf::Clock blockTimer;

        sf::Font font;
        if (!font.loadFromFile("assets/fonts/Arial.ttf")) {
            std::cerr << "Erreur lors du chargement de la police" << std::endl;
            return -1;
        }

        while (window.isOpen()) {
            sf::Event event;
            while (window.pollEvent(event)) {
                if (event.type == sf::Event::Closed)
                    window.close();
            }

            if (!blockPlaced) {
                updateBlockPosition(movingBlock, movingRight);

                int decision = ai.makeDecision(getOverlapWidth(movingBlock, blocks.back()), blockSpeed, movingBlock.getPosition().x);
                if (decision == 1) {
                    handleClick(movingBlock, blocks, blockPlaced, gameOver, ai, blockTimer);
                }

                if (blockTimer.getElapsedTime().asSeconds() >= timeLimit) {
                    ai.train(getOverlapWidth(movingBlock, blocks.back()), blockSpeed, movingBlock.getPosition().x, -5);
                    if (totalReward >= lastReward){
                        lastReward = totalReward;
                        ai.saveModel();
                        std::cout << "Total Reward: " << totalReward <<  std::endl;
                        std::cout << "Score: " << score << std::endl;
                        std::cout << "Sauvegarde" << std::endl;
                        ai.loadModel();
                    }
                    resetGame(blocks, movingBlock, blockPlaced, gameOver);
                    blockTimer.restart();
                }
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

            sf::Text scoreText;
            scoreText.setFont(font);
            scoreText.setString("Score: " + std::to_string(score));
            scoreText.setCharacterSize(30);
            scoreText.setFillColor(sf::Color::White);
            scoreText.setPosition(10, 10);
            window.draw(scoreText);

            if (gameOver) {
                resetGame(blocks, movingBlock, blockPlaced, gameOver);
                blockTimer.restart();
            }

            window.display();
        }

    } catch (const std::exception& e) {
        std::cerr << "Une erreur est survenue: " << e.what() << std::endl;
    }

    return 0;
}
