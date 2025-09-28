/*

author: amelia Gannon

*/

#include <algorithm>
#include <array>
#include <chrono>
#include <cctype>
#include <iostream>
#include <random>
#include <string>
#include <vector>

// ------------------------------------------------------------
// Card / Deck / Hand model
// ------------------------------------------------------------
enum class Suit { Clubs, Diamonds, Hearts, Spades };
enum class Rank {
    Two=2, Three, Four, Five, Six, Seven, Eight, Nine, Ten,
    Jack=11, Queen=12, King=13, Ace=14
};

struct Card {
    Rank rank;
    Suit suit;
};

static std::string rankStr(Rank r) {
    switch (r) {
        case Rank::Two:   return "2";
        case Rank::Three: return "3";
        case Rank::Four:  return "4";
        case Rank::Five:  return "5";
        case Rank::Six:   return "6";
        case Rank::Seven: return "7";
        case Rank::Eight: return "8";
        case Rank::Nine:  return "9";
        case Rank::Ten:   return "10";
        case Rank::Jack:  return "J";
        case Rank::Queen: return "Q";
        case Rank::King:  return "K";
        case Rank::Ace:   return "A";
    }
    return "?";
}

static std::string suitStr(Suit s) {
    // Use simple ASCII fallback if your terminal doesn't like UTF-8 suits.
    switch (s) {
        case Suit::Clubs:    return "♣";
        case Suit::Diamonds: return "♦";
        case Suit::Hearts:   return "♥";
        case Suit::Spades:   return "♠";
    }
    return "?";
}

static std::string cardStr(const Card& c) {
    return rankStr(c.rank) + suitStr(c.suit);
}

class Deck {
public:
    Deck() { reset(); }

    void reset() {
        cards_.clear();
        for (int s = 0; s < 4; ++s) {
            for (int r = 2; r <= 14; ++r) {
                cards_.push_back(Card{static_cast<Rank>(r), static_cast<Suit>(s)});
            }
        }
        shuffle();
        next_ = 0;
    }

    void shuffle() {
        static std::mt19937 rng = std::mt19937(
            static_cast<uint32_t>(std::chrono::high_resolution_clock::now().time_since_epoch().count()));
        std::shuffle(cards_.begin(), cards_.end(), rng);
    }

    Card deal() {
        if (next_ >= cards_.size()) { // auto-reshoe
            reset();
        }
        return cards_[next_++];
    }

private:
    std::vector<Card> cards_;
    size_t next_ = 0;
};

class Hand {
public:
    void clear() { cards_.clear(); }
    void add(const Card& c) { cards_.push_back(c); }

    // Blackjack value: Aces can be 11 or 1 to avoid bust.
    int value() const {
        int total = 0;
        int aces = 0;
        for (auto& c : cards_) {
            int v = 0;
            switch (c.rank) {
                case Rank::Two:   v = 2; break;
                case Rank::Three: v = 3; break;
                case Rank::Four:  v = 4; break;
                case Rank::Five:  v = 5; break;
                case Rank::Six:   v = 6; break;
                case Rank::Seven: v = 7; break;
                case Rank::Eight: v = 8; break;
                case Rank::Nine:  v = 9; break;
                case Rank::Ten:   v = 10; break;
                case Rank::Jack:  v = 10; break;
                case Rank::Queen: v = 10; break;
                case Rank::King:  v = 10; break;
                case Rank::Ace:   v = 11; ++aces; break;
            }
            total += v;
        }
        while (total > 21 && aces > 0) { // reduce Aces from 11 to 1 as needed
            total -= 10;
            --aces;
        }
        return total;
    }

    bool isBlackjack() const { return cards_.size() == 2 && value() == 21; }
    bool isBust() const { return value() > 21; }

    std::string toString(bool hideFirst=false) const {
        std::string out;
        for (size_t i = 0; i < cards_.size(); ++i) {
            if (i == 0 && hideFirst) out += "?? ";
            else out += cardStr(cards_[i]) + " ";
        }
        return out;
    }

private:
    std::vector<Card> cards_;
};

// ------------------------------------------------------------
// Game logic
// ------------------------------------------------------------
class Blackjack {
public:
    void run() {
        std::cout << "=== Terminal Blackjack ===\n";
        std::cout << "Rules: Dealer hits to 17 (stands on soft 17). Blackjack pays 3:2.\n";
        std::cout << "Controls: (H)it, (S)tand, (Q)uit round, ENTER to confirm.\n\n";

        int bankroll = 100;
        int wins=0, losses=0, pushes=0;

        while (true) {
            if (bankroll <= 0) {
                std::cout << "You are out of funds. Resetting bankroll to 100.\n";
                bankroll = 100;
            }

            int bet = promptBet(bankroll);
            if (bet == 0) {
                std::cout << "Exiting game. Final record: W:" << wins << " L:" << losses
                          << " P:" << pushes << " | Bankroll: " << bankroll << "\n";
                break;
            }

            player_.clear(); dealer_.clear();
            // Initial deal
            player_.add(deck_.deal());
            dealer_.add(deck_.deal());
            player_.add(deck_.deal());
            dealer_.add(deck_.deal());

            // Show initial state
            showTable(bankroll, bet, /*revealDealer=*/false);

            // Check immediate blackjack
            bool playerBJ = player_.isBlackjack();
            bool dealerBJ = dealer_.isBlackjack();

            if (playerBJ || dealerBJ) {
                showTable(bankroll, bet, /*revealDealer=*/true);
                if (playerBJ && dealerBJ) {
                    std::cout << "Both have Blackjack! Push.\n";
                    ++pushes;
                } else if (playerBJ) {
                    int payout = bet + bet * 3 / 2; // return bet + 3:2 win
                    bankroll += payout - bet; // net +1.5*bet
                    std::cout << "Blackjack! You win +" << (payout - bet) << ".\n";
                    ++wins;
                } else {
                    bankroll -= bet;
                    std::cout << "Dealer Blackjack. You lose -" << bet << ".\n";
                    ++losses;
                }
                if (!playAgain()) break;
                continue;
            }

            // Player turn
            bool playerQuit = false;
            while (true) {
                char action = promptAction();
                if (action == 'H') {
                    player_.add(deck_.deal());
                    showTable(bankroll, bet, /*revealDealer=*/false);
                    if (player_.isBust()) {
                        std::cout << "You bust.\n";
                        break;
                    }
                } else if (action == 'S') {
                    break;
                } else if (action == 'Q') {
                    playerQuit = true;
                    break;
                }
            }

            if (playerQuit) {
                std::cout << "Round aborted. No money exchanged.\n";
                if (!playAgain()) break;
                continue;
            }

            if (player_.isBust()) {
                bankroll -= bet;
                ++losses;
                std::cout << "You lose -" << bet << ".\n";
                if (!playAgain()) break;
                continue;
            }

            // Dealer turn (reveal hidden card)
            showTable(bankroll, bet, /*revealDealer=*/true);
            dealerPlay();

            // Resolve
            showTable(bankroll, bet, /*revealDealer=*/true);
            int pv = player_.value(), dv = dealer_.value();
            if (dealer_.isBust()) {
                bankroll -= -bet; // bankroll += bet;
                bankroll += bet;
                ++wins;
                std::cout << "Dealer busts. You win +" << bet << ".\n";
            } else if (pv > dv) {
                bankroll += bet;
                ++wins;
                std::cout << "You win +" << bet << ".\n";
            } else if (pv < dv) {
                bankroll -= bet;
                ++losses;
                std::cout << "You lose -" << bet << ".\n";
            } else {
                ++pushes;
                std::cout << "Push. Bet returned.\n";
            }

            if (!playAgain()) break;
        }
    }

private:
    Deck deck_;
    Hand player_, dealer_;

    int promptBet(int bankroll) {
        while (true) {
            std::cout << "Bankroll: " << bankroll << " | Enter bet (1.." << bankroll
                      << "), or 0 to quit: ";
            std::string s;
            if (!std::getline(std::cin, s)) return 0;
            if (s.empty()) continue;
            bool ok = true;
            for (char c : s) if (!std::isdigit(static_cast<unsigned char>(c))) ok = false;
            if (!ok) { std::cout << "Enter digits only.\n"; continue; }
            int b = std::stoi(s);
            if (b >= 0 && b <= bankroll) return b;
            std::cout << "Bet must be between 0 and " << bankroll << ".\n";
        }
    }

    char promptAction() {
        while (true) {
            std::cout << "(H)it, (S)tand, (Q)uit round: ";
            std::string s;
            std::getline(std::cin, s);
            if (s.empty()) continue;
            char c = std::toupper(static_cast<unsigned char>(s[0]));
            if (c=='H' || c=='S' || c=='Q') return c;
            std::cout << "Please enter H, S, or Q.\n";
        }
    }

    void dealerPlay() {
        // Dealer hits until value >= 17. Stands on soft 17 (common casino rule varies).
        while (dealer_.value() < 17) {
            dealer_.add(deck_.deal());
        }
    }

    void showTable(int bankroll, int bet, bool revealDealer) {
        std::cout << "\n----------------------------------------\n";
        std::cout << "Dealer: " << dealer_.toString(!revealDealer);
        if (revealDealer) std::cout << " (" << dealer_.value() << ")";
        std::cout << "\n";
        std::cout << "Player: " << player_.toString(false)
                  << " (" << player_.value() << ")\n";
        std::cout << "Bet: " << bet << " | Bankroll: " << bankroll << "\n";
        std::cout << "----------------------------------------\n";
    }

    bool playAgain() {
        while (true) {
            std::cout << "Play another round? (Y/N): ";
            std::string s;
            std::getline(std::cin, s);
            if (s.empty()) continue;
            char c = std::toupper(static_cast<unsigned char>(s[0]));
            if (c == 'Y') return true;
            if (c == 'N') return false;
            std::cout << "Please enter Y or N.\n";
        }
    }
};

// ------------------------------------------------------------
// Entry point
// ------------------------------------------------------------
int main() {
    Blackjack game;
    game.run();
    return 0;
}
