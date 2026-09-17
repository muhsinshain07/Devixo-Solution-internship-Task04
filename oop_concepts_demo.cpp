#include <exception>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

using namespace std;

class AccountException : public runtime_error {
public:
    explicit AccountException(const string& message) : runtime_error(message) {}
};

class DemoAccount {
private:
    string owner_;
    double balance_;

protected:
    void setBalance(double balance) { balance_ = balance; }

public:
    DemoAccount(string owner, double balance)
        : owner_(move(owner)), balance_(balance) {}

    virtual ~DemoAccount() = default;

    const string& owner() const { return owner_; }
    double balance() const { return balance_; }

    virtual string accountType() const = 0;

    virtual void withdraw(double amount) {
        if (amount <= 0.0) {
            throw AccountException("Withdrawal amount must be positive.");
        }
        if (amount > balance_) {
            throw AccountException("Insufficient balance.");
        }
        balance_ -= amount;
    }

    virtual void print() const {
        cout << accountType() << " account | Owner: " << owner_
             << " | Balance: " << balance_ << '\n';
    }
};

class DemoSavingsAccount final : public DemoAccount {
public:
    DemoSavingsAccount(string owner, double balance)
        : DemoAccount(move(owner), balance) {}

    string accountType() const override { return "Savings"; }

    void withdraw(double amount) override {
        if (balance() - amount < 500.0) {
            throw AccountException("Savings account must retain at least 500.00.");
        }
        DemoAccount::withdraw(amount);
    }
};

class DemoCurrentAccount final : public DemoAccount {
public:
    DemoCurrentAccount(string owner, double balance)
        : DemoAccount(move(owner), balance) {}

    string accountType() const override { return "Current"; }
};

int main() {
    vector<unique_ptr<DemoAccount>> accounts;
    accounts.push_back(make_unique<DemoSavingsAccount>("Alice", 1500.0));
    accounts.push_back(make_unique<DemoCurrentAccount>("Bob", 200.0));

    cout << "Polymorphic account display:\n";
    for (const auto& account : accounts) account->print();

    try {
        accounts[0]->withdraw(1200.0);
    } catch (const AccountException& error) {
        cout << "Handled exception: " << error.what() << '\n';
    }

    accounts[1]->withdraw(100.0);
    cout << "After a valid withdrawal:\n";
    accounts[1]->print();

    return 0;
}
