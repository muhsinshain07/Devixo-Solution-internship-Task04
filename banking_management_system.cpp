#include <algorithm>
#include <chrono>
#include <cctype>
#include <cstdlib>
#include <ctime>
#include <exception>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace std;

// ----------------------------- Utility Functions -----------------------------
namespace Input {
    void clearLine() {
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
    }

    string readLine(const string& prompt, bool allowEmpty = false) {
        while (true) {
            cout << prompt;
            string value;
            getline(cin, value);
            if (allowEmpty || !value.empty()) return value;
            cout << "Input cannot be empty. Please try again.\n";
        }
    }

    int readInt(const string& prompt, int minValue, int maxValue) {
        while (true) {
            cout << prompt;
            int value;
            if (cin >> value && value >= minValue && value <= maxValue) {
                clearLine();
                return value;
            }
            cout << "Please enter a number between " << minValue << " and " << maxValue << ".\n";
            clearLine();
        }
    }

    double readDouble(const string& prompt, double minValue) {
        while (true) {
            cout << prompt;
            double value;
            if (cin >> value && value >= minValue) {
                clearLine();
                return value;
            }
            cout << "Please enter a valid amount of at least " << fixed << setprecision(2)
                 << minValue << ".\n";
            clearLine();
        }
    }

    bool confirm(const string& prompt) {
        while (true) {
            string answer = readLine(prompt + " (y/n): ");
            transform(answer.begin(), answer.end(), answer.begin(),
                      [](unsigned char c) { return static_cast<char>(tolower(c)); });
            if (answer == "y" || answer == "yes") return true;
            if (answer == "n" || answer == "no") return false;
            cout << "Please enter y or n.\n";
        }
    }
}

namespace Format {
    string dateTime() {
        const time_t now = time(nullptr);
        tm localTime{};
        if (const tm* currentLocalTime = localtime(&now)) {
            localTime = *currentLocalTime;
        }
        ostringstream output;
        output << put_time(&localTime, "%Y-%m-%d %H:%M:%S");
        return output.str();
    }

    string money(double amount) {
        ostringstream output;
        output << fixed << setprecision(2) << amount;
        return output.str();
    }

    string sanitize(string value) {
        replace(value.begin(), value.end(), '|', '/');
        return value;
    }
}

// ----------------------------- Exceptions -----------------------------
class BankingException : public runtime_error {
public:
    explicit BankingException(const string& message) : runtime_error(message) {}
};

class AccountNotFoundException : public BankingException {
public:
    explicit AccountNotFoundException(const string& message) : BankingException(message) {}
};

class AuthenticationException : public BankingException {
public:
    explicit AuthenticationException(const string& message) : BankingException(message) {}
};

class TransactionException : public BankingException {
public:
    explicit TransactionException(const string& message) : BankingException(message) {}
};

// ----------------------------- Transaction Classes -----------------------------
class Transaction {
private:
    string timestamp_;
    string type_;
    double amount_;
    string description_;
    double balanceAfter_;

public:
    Transaction() : amount_(0.0), balanceAfter_(0.0) {}

    Transaction(string timestamp, string type, double amount,
                string description, double balanceAfter)
        : timestamp_(move(timestamp)), type_(move(type)), amount_(amount),
          description_(move(description)), balanceAfter_(balanceAfter) {}

    const string& getTimestamp() const { return timestamp_; }
    const string& getType() const { return type_; }
    double getAmount() const { return amount_; }
    const string& getDescription() const { return description_; }
    double getBalanceAfter() const { return balanceAfter_; }

    string serialize() const {
        ostringstream output;
        output << timestamp_ << "~" << Format::sanitize(type_) << "~"
               << setprecision(17) << amount_ << "~"
               << Format::sanitize(description_) << "~"
               << balanceAfter_;
        return output.str();
    }

    static Transaction deserialize(const string& data) {
        string timestamp, type, amount, description, balance;
        stringstream input(data);
        if (!getline(input, timestamp, '~') || !getline(input, type, '~') ||
            !getline(input, amount, '~') || !getline(input, description, '~') ||
            !getline(input, balance, '~')) {
            throw BankingException("Invalid transaction record in storage file.");
        }
        return Transaction(timestamp, type, stod(amount), description, stod(balance));
    }

    void display() const {
        cout << left << setw(20) << timestamp_
             << setw(14) << type_
             << right << setw(12) << Format::money(amount_)
             << setw(16) << Format::money(balanceAfter_)
             << "  " << description_ << '\n';
    }
};

// ----------------------------- Polymorphic Account Types -----------------------------
class Account {
protected:
    long long accountNumber_;
    string holderName_;
    string phone_;
    string pin_;
    double balance_;
    vector<Transaction> transactions_;

    Account(long long accountNumber, string holderName, string phone,
            string pin, double balance)
        : accountNumber_(accountNumber), holderName_(move(holderName)),
          phone_(move(phone)), pin_(move(pin)), balance_(balance) {}

    void addTransaction(const string& type, double amount, const string& description) {
        transactions_.emplace_back(Format::dateTime(), type, amount, description, balance_);
    }

public:
    virtual ~Account() = default;

    virtual string getAccountType() const = 0;
    virtual double minimumBalance() const = 0;
    virtual double withdrawalLimit() const = 0;
    virtual string extraData() const { return ""; }
    virtual void displayTypeDetails() const = 0;

    long long getAccountNumber() const { return accountNumber_; }
    const string& getHolderName() const { return holderName_; }
    const string& getPhone() const { return phone_; }
    double getBalance() const { return balance_; }
    const vector<Transaction>& getTransactions() const { return transactions_; }

    void updatePersonalDetails(const string& name, const string& phone) {
        if (!name.empty()) holderName_ = name;
        if (!phone.empty()) phone_ = phone;
    }

    bool authenticate(const string& pin) const { return pin_ == pin; }

    void deposit(double amount, const string& description = "Cash deposit") {
        if (amount <= 0.0) throw TransactionException("Deposit amount must be positive.");
        balance_ += amount;
        addTransaction("DEPOSIT", amount, description);
    }

    void withdraw(double amount, const string& description = "Cash withdrawal") {
        if (amount <= 0.0) throw TransactionException("Withdrawal amount must be positive.");
        if (amount > withdrawalLimit()) {
            throw TransactionException("This withdrawal exceeds the account withdrawal limit.");
        }
        if (balance_ - amount < minimumBalance()) {
            throw TransactionException("Insufficient funds or minimum balance requirement would be violated.");
        }
        balance_ -= amount;
        addTransaction("WITHDRAW", amount, description);
    }

    void recordTransferOut(double amount, long long targetAccount) {
        addTransaction("TRANSFER OUT", amount,
                       "Transfer to account " + to_string(targetAccount));
    }

    void recordTransferIn(double amount, long long sourceAccount) {
        addTransaction("TRANSFER IN", amount,
                       "Transfer from account " + to_string(sourceAccount));
    }

    string serializeTransactions() const {
        ostringstream output;
        for (size_t i = 0; i < transactions_.size(); ++i) {
            if (i > 0) output << ";";
            output << transactions_[i].serialize();
        }
        return output.str();
    }

    void loadTransactions(const string& serialized) {
        transactions_.clear();
        if (serialized.empty()) return;
        string record;
        stringstream input(serialized);
        while (getline(input, record, ';')) {
            if (!record.empty()) transactions_.push_back(Transaction::deserialize(record));
        }
    }

    virtual string serialize() const {
        // Fields are separated by '|'; transaction fields use '~' and records use ';'.
        ostringstream output;
        output << accountNumber_ << "|" << Format::sanitize(holderName_) << "|"
               << Format::sanitize(phone_) << "|" << pin_ << "|"
               << setprecision(17) << balance_ << "|" << getAccountType() << "|"
               << Format::sanitize(extraData()) << "|" << serializeTransactions();
        return output.str();
    }

    virtual void display() const {
        cout << "Account number : " << accountNumber_ << '\n'
             << "Account holder : " << holderName_ << '\n'
             << "Phone          : " << phone_ << '\n'
             << "Account type   : " << getAccountType() << '\n'
             << "Balance        : " << Format::money(balance_) << '\n';
        displayTypeDetails();
    }

    void displayStatement() const {
        display();
        cout << "\nTransaction history\n"
             << left << setw(20) << "Date/time" << setw(14) << "Type"
             << right << setw(12) << "Amount" << setw(16) << "Balance"
             << "  Description\n";
        cout << string(90, '-') << '\n';
        if (transactions_.empty()) {
            cout << "No transactions recorded.\n";
        } else {
            for (const auto& transaction : transactions_) transaction.display();
        }
    }
};

class SavingsAccount final : public Account {
private:
    double interestRate_;

public:
    SavingsAccount(long long number, string name, string phone, string pin,
                   double balance, double interestRate = 3.0)
        : Account(number, move(name), move(phone), move(pin), balance),
          interestRate_(interestRate) {}

    string getAccountType() const override { return "SAVINGS"; }
    double minimumBalance() const override { return 500.0; }
    double withdrawalLimit() const override { return 50000.0; }
    string extraData() const override { return to_string(interestRate_); }

    void displayTypeDetails() const override {
        cout << "Interest rate  : " << fixed << setprecision(2) << interestRate_ << "%\n";
        cout << "Minimum balance: " << Format::money(minimumBalance()) << '\n';
    }
};

class CurrentAccount final : public Account {
private:
    double overdraftLimit_;

public:
    CurrentAccount(long long number, string name, string phone, string pin,
                   double balance, double overdraftLimit = 10000.0)
        : Account(number, move(name), move(phone), move(pin), balance),
          overdraftLimit_(overdraftLimit) {}

    string getAccountType() const override { return "CURRENT"; }
    double minimumBalance() const override { return -overdraftLimit_; }
    double withdrawalLimit() const override { return 100000.0; }
    string extraData() const override { return to_string(overdraftLimit_); }

    void displayTypeDetails() const override {
        cout << "Overdraft limit: " << Format::money(overdraftLimit_) << '\n';
    }
};

// ----------------------------- Bank Management -----------------------------
class Bank {
private:
    vector<unique_ptr<Account>> accounts_;
    const string accountsFile_ = "accounts.dat";
    long long nextAccountNumber_ = 100001;

    Account* findAccount(long long number) {
        auto it = find_if(accounts_.begin(), accounts_.end(),
                          [number](const auto& account) {
                              return account->getAccountNumber() == number;
                          });
        return it == accounts_.end() ? nullptr : it->get();
    }

    const Account* findAccount(long long number) const {
        auto it = find_if(accounts_.begin(), accounts_.end(),
                          [number](const auto& account) {
                              return account->getAccountNumber() == number;
                          });
        return it == accounts_.end() ? nullptr : it->get();
    }

    static vector<string> split(const string& line, char delimiter) {
        vector<string> fields;
        string field;
        stringstream input(line);
        while (getline(input, field, delimiter)) fields.push_back(field);
        return fields;
    }

    unique_ptr<Account> deserializeAccount(const string& line) {
        const auto fields = split(line, '|');
        if (fields.size() < 8) throw BankingException("Invalid account record.");

        const long long number = stoll(fields[0]);
        const string& name = fields[1];
        const string& phone = fields[2];
        const string& pin = fields[3];
        const double balance = stod(fields[4]);
        const string& type = fields[5];
        const double extra = fields[6].empty() ? 0.0 : stod(fields[6]);

        unique_ptr<Account> account;
        if (type == "SAVINGS") {
            account = make_unique<SavingsAccount>(number, name, phone, pin, balance,
                                                  extra == 0.0 ? 3.0 : extra);
        } else if (type == "CURRENT") {
            account = make_unique<CurrentAccount>(number, name, phone, pin, balance,
                                                  extra == 0.0 ? 10000.0 : extra);
        } else {
            throw BankingException("Unknown account type in storage file.");
        }
        account->loadTransactions(fields[7]);
        return account;
    }

public:
    Bank() { load(); }

    void load() {
        ifstream input(accountsFile_);
        if (!input) return;

        string line;
        while (getline(input, line)) {
            if (line.empty()) continue;
            try {
                auto account = deserializeAccount(line);
                nextAccountNumber_ = max(nextAccountNumber_, account->getAccountNumber() + 1);
                accounts_.push_back(move(account));
            } catch (const exception& error) {
                cerr << "Warning: skipped invalid account record: " << error.what() << '\n';
            }
        }
    }

    void save() const {
        const string temporaryFile = accountsFile_ + ".tmp";
        ofstream output(temporaryFile, ios::trunc);
        if (!output) throw BankingException("Unable to open account storage for writing.");
        for (const auto& account : accounts_) output << account->serialize() << '\n';
        output.close();
        if (output.fail()) throw BankingException("Unable to save account data.");
        if (remove(accountsFile_.c_str()) != 0 && ifstream(accountsFile_).good()) {
            throw BankingException("Unable to replace the account storage file.");
        }
        if (rename(temporaryFile.c_str(), accountsFile_.c_str()) != 0) {
            throw BankingException("Unable to finalize the account storage file.");
        }
    }

    long long createAccount(const string& type, const string& name,
                            const string& phone, const string& pin, double initialDeposit) {
        if (initialDeposit < 0.0) throw BankingException("Initial deposit cannot be negative.");
        if (type != "SAVINGS" && type != "CURRENT") throw BankingException("Invalid account type.");

        const long long number = nextAccountNumber_++;
        unique_ptr<Account> account;
        if (type == "SAVINGS") {
            if (initialDeposit < 500.0) throw BankingException("Savings accounts require at least 500.00.");
            account = make_unique<SavingsAccount>(number, name, phone, pin, initialDeposit);
        } else {
            account = make_unique<CurrentAccount>(number, name, phone, pin, initialDeposit);
        }
        if (initialDeposit > 0.0) account->deposit(initialDeposit, "Opening deposit");
        accounts_.push_back(move(account));
        save();
        return number;
    }

    Account& requireAccount(long long number) {
        Account* account = findAccount(number);
        if (!account) throw AccountNotFoundException("Account not found.");
        return *account;
    }

    const Account& requireAccount(long long number) const {
        const Account* account = findAccount(number);
        if (!account) throw AccountNotFoundException("Account not found.");
        return *account;
    }

    void authenticate(const Account& account, const string& pin) const {
        if (!account.authenticate(pin)) throw AuthenticationException("Incorrect PIN.");
    }

    void updateAccount(long long number, const string& name, const string& phone) {
        requireAccount(number).updatePersonalDetails(name, phone);
        save();
    }

    void deleteAccount(long long number, const string& pin) {
        Account& account = requireAccount(number);
        authenticate(account, pin);
        if (account.getBalance() != 0.0) {
            throw BankingException("An account can only be deleted when its balance is zero.");
        }
        accounts_.erase(remove_if(accounts_.begin(), accounts_.end(),
                                   [number](const auto& item) {
                                       return item->getAccountNumber() == number;
                                   }), accounts_.end());
        save();
    }

    void deposit(long long number, double amount) {
        requireAccount(number).deposit(amount);
        save();
    }

    void withdraw(long long number, double amount, const string& pin) {
        Account& account = requireAccount(number);
        authenticate(account, pin);
        account.withdraw(amount);
        save();
    }

    void transfer(long long sourceNumber, long long targetNumber, double amount,
                  const string& pin) {
        if (sourceNumber == targetNumber) throw TransactionException("Source and target accounts must differ.");
        Account& source = requireAccount(sourceNumber);
        Account& target = requireAccount(targetNumber);
        authenticate(source, pin);
        source.withdraw(amount, "Transfer to account " + to_string(targetNumber));
        source.recordTransferOut(amount, targetNumber);
        target.deposit(amount, "Transfer from account " + to_string(sourceNumber));
        target.recordTransferIn(amount, sourceNumber);
        save();
    }

    vector<const Account*> search(const string& query) const {
        vector<const Account*> results;
        for (const auto& account : accounts_) {
            if (to_string(account->getAccountNumber()).find(query) != string::npos ||
                account->getHolderName().find(query) != string::npos ||
                account->getPhone().find(query) != string::npos) {
                results.push_back(account.get());
            }
        }
        return results;
    }

    vector<const Account*> sortedByName() const {
        vector<const Account*> results;
        for (const auto& account : accounts_) results.push_back(account.get());
        sort(results.begin(), results.end(), [](const Account* left, const Account* right) {
            return left->getHolderName() < right->getHolderName();
        });
        return results;
    }

    vector<const Account*> sortedByBalance() const {
        vector<const Account*> results;
        for (const auto& account : accounts_) results.push_back(account.get());
        sort(results.begin(), results.end(), [](const Account* left, const Account* right) {
            return left->getBalance() > right->getBalance();
        });
        return results;
    }

    void displayAccountList(const vector<const Account*>& list) const {
        if (list.empty()) {
            cout << "No accounts found.\n";
            return;
        }
        cout << left << setw(12) << "Account" << setw(24) << "Holder"
             << setw(14) << "Type" << right << setw(16) << "Balance" << '\n';
        cout << string(66, '-') << '\n';
        for (const Account* account : list) {
            cout << left << setw(12) << account->getAccountNumber()
                 << setw(24) << account->getHolderName()
                 << setw(14) << account->getAccountType()
                 << right << setw(16) << Format::money(account->getBalance()) << '\n';
        }
    }

    void viewAll() const {
        vector<const Account*> list;
        for (const auto& account : accounts_) list.push_back(account.get());
        displayAccountList(list);
    }

    size_t size() const { return accounts_.size(); }
};

// ----------------------------- User Interface -----------------------------
class BankingApplication {
private:
    Bank bank_;

    static void header(const string& title) {
        cout << "\n========================================\n"
             << title << "\n"
             << "========================================\n";
    }

    long long readAccountNumber(const string& prompt = "Account number: ") {
        return Input::readInt(prompt, 100000, 999999999);
    }

    string readPin() {
        while (true) {
            string pin = Input::readLine("4-digit PIN: ");
            if (pin.size() == 4 && all_of(pin.begin(), pin.end(), ::isdigit)) return pin;
            cout << "PIN must contain exactly four digits.\n";
        }
    }

    void createAccount() {
        header("Create Account");
        const int typeChoice = Input::readInt("1. Savings\n2. Current\nChoose type: ", 1, 2);
        const string name = Input::readLine("Account holder name: ");
        const string phone = Input::readLine("Phone number: ");
        const string pin = readPin();
        const double initialDeposit = Input::readDouble("Initial deposit: ", typeChoice == 1 ? 500.0 : 0.0);
        const long long number = bank_.createAccount(typeChoice == 1 ? "SAVINGS" : "CURRENT",
                                                     name, phone, pin, initialDeposit);
        cout << "Account created successfully. Account number: " << number << '\n';
    }

    void viewAccount() {
        header("View Account");
        const long long number = readAccountNumber();
        bank_.requireAccount(number).display();
    }

    void searchAccounts() {
        header("Search Accounts");
        const string query = Input::readLine("Enter account number, name, or phone: ");
        bank_.displayAccountList(bank_.search(query));
    }

    void updateAccount() {
        header("Update Account");
        const long long number = readAccountNumber();
        const Account& account = bank_.requireAccount(number);
        bank_.authenticate(account, readPin());
        const string name = Input::readLine("New name (leave empty to keep current): ", true);
        const string phone = Input::readLine("New phone (leave empty to keep current): ", true);
        bank_.updateAccount(number, name, phone);
        cout << "Account updated successfully.\n";
    }

    void deleteAccount() {
        header("Delete Account");
        const long long number = readAccountNumber();
        const string pin = readPin();
        if (Input::confirm("Delete this account permanently?")) {
            bank_.deleteAccount(number, pin);
            cout << "Account deleted successfully.\n";
        } else {
            cout << "Deletion cancelled.\n";
        }
    }

    void deposit() {
        header("Deposit");
        const long long number = readAccountNumber();
        const double amount = Input::readDouble("Deposit amount: ", 0.01);
        bank_.deposit(number, amount);
        cout << "Deposit completed.\n";
    }

    void withdraw() {
        header("Withdraw");
        const long long number = readAccountNumber();
        const string pin = readPin();
        const double amount = Input::readDouble("Withdrawal amount: ", 0.01);
        bank_.withdraw(number, amount, pin);
        cout << "Withdrawal completed.\n";
    }

    void transfer() {
        header("Transfer");
        const long long source = readAccountNumber("Source account number: ");
        const string pin = readPin();
        const long long target = readAccountNumber("Target account number: ");
        const double amount = Input::readDouble("Transfer amount: ", 0.01);
        bank_.transfer(source, target, amount, pin);
        cout << "Transfer completed.\n";
    }

    void balanceCheck() {
        header("Balance Check");
        const long long number = readAccountNumber();
        const Account& account = bank_.requireAccount(number);
        bank_.authenticate(account, readPin());
        cout << "Current balance: " << Format::money(account.getBalance()) << '\n';
    }

    void statement() {
        header("Account Statement");
        const long long number = readAccountNumber();
        const Account& account = bank_.requireAccount(number);
        bank_.authenticate(account, readPin());
        account.displayStatement();
    }

    void sorting() {
        header("Sort Accounts");
        const int choice = Input::readInt("1. Sort by holder name\n2. Sort by balance (highest first)\nChoose: ", 1, 2);
        bank_.displayAccountList(choice == 1 ? bank_.sortedByName() : bank_.sortedByBalance());
    }

public:
    void run() {
        cout << "\nWelcome to the Banking Management System\n"
             << "Persistent storage file: accounts.dat\n";
        while (true) {
            header("Main Menu");
            cout << "1. Create account\n"
                 << "2. View account\n"
                 << "3. View all accounts\n"
                 << "4. Search accounts\n"
                 << "5. Update account\n"
                 << "6. Delete account\n"
                 << "7. Deposit\n"
                 << "8. Withdraw\n"
                 << "9. Transfer\n"
                 << "10. Balance check\n"
                 << "11. Account statement\n"
                 << "12. Sort accounts\n"
                 << "0. Exit\n";
            const int choice = Input::readInt("Choose an option: ", 0, 12);
            try {
                switch (choice) {
                    case 1: createAccount(); break;
                    case 2: viewAccount(); break;
                    case 3: header("All Accounts"); bank_.viewAll(); break;
                    case 4: searchAccounts(); break;
                    case 5: updateAccount(); break;
                    case 6: deleteAccount(); break;
                    case 7: deposit(); break;
                    case 8: withdraw(); break;
                    case 9: transfer(); break;
                    case 10: balanceCheck(); break;
                    case 11: statement(); break;
                    case 12: sorting(); break;
                    case 0: cout << "Data saved. Goodbye.\n"; return;
                }
            } catch (const BankingException& error) {
                cout << "Operation failed: " << error.what() << '\n';
            } catch (const exception& error) {
                cout << "Unexpected error: " << error.what() << '\n';
            }
            cout << '\n';
        }
    }
};

int main() {
    try {
        BankingApplication application;
        application.run();
    } catch (const exception& error) {
        cerr << "Fatal error: " << error.what() << '\n';
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
