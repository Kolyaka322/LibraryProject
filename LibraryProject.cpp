#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <stdexcept>

class LibraryException : public std::runtime_error {
public:
    explicit LibraryException(const std::string& message)
        : std::runtime_error(message) {}
};

class BookNotFoundException : public LibraryException {
private:
    std::string identifier;
    bool byTitle;

    static std::string buildMessage(const std::string& id, bool isTitle) {
        std::ostringstream oss;
        oss << "[BookNotFoundException] Книгу не знайдено за "
            << (isTitle ? "назвою: \"" : "ISBN: \"") << id << "\"";
        return oss.str();
    }

public:
    BookNotFoundException(const std::string& identifier, bool byTitle = false)
        : LibraryException(buildMessage(identifier, byTitle)),
        identifier(identifier),
        byTitle(byTitle) {
    }

    const std::string& getIdentifier() const noexcept { return identifier; }
    bool isSearchedByTitle() const noexcept { return byTitle; }
};

class BookUnavailableException : public LibraryException {
private:
    std::string isbn;
    int totalCopies;
    std::string nearestReturnDate;

    static std::string buildMessage(const std::string& isbn, int total, const std::string& returnDate) {
        std::ostringstream oss;
        oss << "[BookUnavailableException] Усі примірники зайняті (ISBN: " << isbn
            << ", всього примірників: " << total
            << ", найближче повернення: " << returnDate << ")";
        return oss.str();
    }

public:
    BookUnavailableException(const std::string& isbn, int totalCopies, const std::string& nearestReturnDate)
        : LibraryException(buildMessage(isbn, totalCopies, nearestReturnDate)),
        isbn(isbn),
        totalCopies(totalCopies),
        nearestReturnDate(nearestReturnDate) {
    }

    const std::string& getIsbn() const noexcept { return isbn; }
    int getTotalCopies() const noexcept { return totalCopies; }
    const std::string& getNearestReturnDate() const noexcept { return nearestReturnDate; }
};

class UserNotFoundException : public LibraryException {
private:
    std::string userID;

    static std::string buildMessage(const std::string& id) {
        std::ostringstream oss;
        oss << "[UserNotFoundException] Читача з ID \"" << id << "\" не знайдено в системі";
        return oss.str();
    }

public:
    explicit UserNotFoundException(const std::string& userID)
        : LibraryException(buildMessage(userID)),
        userID(userID) {
    }

    const std::string& getUserID() const noexcept { return userID; }
};

class OverdueException : public LibraryException {
private:
    std::string isbn;
    std::string borrowDate;
    std::string dueDate;
    double fineAmount;

    static std::string buildMessage(const std::string& isbn,
        const std::string& borrow,
        const std::string& due,
        double fine) {
        std::ostringstream oss;
        oss << "[OverdueException] Прострочене повернення (ISBN: " << isbn
            << ", видано: " << borrow
            << ", термін до: " << due
            << ", штраф: " << std::fixed << std::setprecision(2) << fine << " грн)";
        return oss.str();
    }

public:
    OverdueException(const std::string& isbn,
        const std::string& borrowDate,
        const std::string& dueDate,
        double fineAmount)
        : LibraryException(buildMessage(isbn, borrowDate, dueDate, fineAmount)),
        isbn(isbn),
        borrowDate(borrowDate),
        dueDate(dueDate),
        fineAmount(fineAmount) {
    }

    const std::string& getIsbn() const noexcept { return isbn; }
    const std::string& getBorrowDate() const noexcept { return borrowDate; }
    const std::string& getDueDate() const noexcept { return dueDate; }
    double getFineAmount() const noexcept { return fineAmount; }
};

struct Book {
    std::string isbn;
    std::string title;
    int totalCopies;
    int availableCopies;
};

struct BorrowRecord {
    std::string userID;
    std::string isbn;
    std::string borrowDate;
    std::string dueDate;
    int overdueDays;
};

class Library {
private:
    static constexpr double DAILY_FINE = 15.0;

    std::map<std::string, Book> catalog;
    std::vector<std::string> registeredUsers;
    std::vector<BorrowRecord> activeLoans;

    bool hasUser(const std::string& userID) const {
        for (const auto& id : registeredUsers) {
            if (id == userID) return true;
        }
        return false;
    }

public:
    void addUser(const std::string& userID) {
        registeredUsers.push_back(userID);
    }

    void addBook(const Book& book) {
        catalog[book.isbn] = book;
    }

    void addActiveLoan(const BorrowRecord& record) {
        activeLoans.push_back(record);
    }

    const Book& findBook(const std::string& isbn) const {
        auto it = catalog.find(isbn);
        if (it == catalog.end()) {
            throw BookNotFoundException(isbn, false);
        }
        return it->second;
    }

    const Book& findBookByTitle(const std::string& title) const {
        for (const auto& pair : catalog) {
            if (pair.second.title == title) {
                return pair.second;
            }
        }
        throw BookNotFoundException(title, true);
    }

    void borrowBook(const std::string& userID, const std::string& isbn) {
        if (!hasUser(userID)) {
            throw UserNotFoundException(userID);
        }

        auto it = catalog.find(isbn);
        if (it == catalog.end()) {
            throw BookNotFoundException(isbn, false);
        }

        Book& book = it->second;
        if (book.availableCopies <= 0) {
            std::string nearestDue = "2026-12-01";
            bool foundLoan = false;
            for (const auto& loan : activeLoans) {
                if (loan.isbn == isbn) {
                    if (!foundLoan || loan.dueDate < nearestDue) {
                        nearestDue = loan.dueDate;
                        foundLoan = true;
                    }
                }
            }
            throw BookUnavailableException(isbn, book.totalCopies, nearestDue);
        }

        book.availableCopies -= 1;
        BorrowRecord record{
            userID,
            isbn,
            "2026-11-24",
            "2026-12-08",
            0
        };
        activeLoans.push_back(record);
    }

    void returnBook(const std::string& userID, const std::string& isbn) {
        if (!hasUser(userID)) {
            throw UserNotFoundException(userID);
        }

        auto bookIt = catalog.find(isbn);
        if (bookIt == catalog.end()) {
            throw BookNotFoundException(isbn, false);
        }

        for (auto it = activeLoans.begin(); it != activeLoans.end(); ++it) {
            if (it->userID == userID && it->isbn == isbn) {
                if (it->overdueDays > 0) {
                    double fine = it->overdueDays * DAILY_FINE;
                    throw OverdueException(isbn, it->borrowDate, it->dueDate, fine);
                }
                bookIt->second.availableCopies += 1;
                activeLoans.erase(it);
                return;
            }
        }

        throw LibraryException("Активного запису про видачу книги (ISBN: " + isbn +
            ") користувачу \"" + userID + "\" не знайдено");
    }
};

template <typename Action>
void runScenario(const std::string& title, Action&& action) {
    std::cout << "=== Сценарій: " << title << " ===\n";
    try {
        action();
        std::cout << "Операцію виконано успішно.\n";
    }
    catch (const BookNotFoundException& ex) {
        std::cout << "Перехоплено [BookNotFoundException]: " << ex.what() << "\n"
            << "  -> Контекст: пошук за " << (ex.isSearchedByTitle() ? "назвою" : "ISBN")
            << ", ключ = " << ex.getIdentifier() << "\n";
    }
    catch (const BookUnavailableException& ex) {
        std::cout << "Перехоплено [BookUnavailableException]: " << ex.what() << "\n"
            << "  -> Контекст: ISBN = " << ex.getIsbn()
            << ", всього = " << ex.getTotalCopies()
            << ", очікувана дата = " << ex.getNearestReturnDate() << "\n";
    }
    catch (const UserNotFoundException& ex) {
        std::cout << "Перехоплено [UserNotFoundException]: " << ex.what() << "\n"
            << "  -> Контекст: userID = " << ex.getUserID() << "\n";
    }
    catch (const OverdueException& ex) {
        std::cout << "Перехоплено [OverdueException]: " << ex.what() << "\n"
            << "  -> Контекст: ISBN = " << ex.getIsbn()
            << ", видано = " << ex.getBorrowDate()
            << ", граничний строк = " << ex.getDueDate()
            << ", штраф = " << ex.getFineAmount() << " грн\n";
    }
    catch (const LibraryException& ex) {
        std::cout << "Перехоплено загальний [LibraryException]: " << ex.what() << "\n";
    }
    catch (const std::exception& ex) {
        std::cout << "Перехоплено стандартний [std::exception]: " << ex.what() << "\n";
    }
    std::cout << "\n";
}

int main() {
    system("chcp 1251>null");
    Library lib;
    lib.addUser("U-101");
    lib.addUser("U-102");

    lib.addBook({ "978-0131103627", "The C Programming Language", 3, 3 });
    lib.addBook({ "978-0201700732", "The C++ Programming Language", 2, 0 });
    lib.addBook({ "978-0262033848", "Introduction to Algorithms", 2, 1 });

    lib.addActiveLoan({
        "U-101",
        "978-0262033848",
        "2026-11-04",
        "2026-11-18",
        6
        });

    lib.addActiveLoan({
        "U-102",
        "978-0201700732",
        "2026-11-13",
        "2026-11-27",
        0
        });

    runScenario("Неіснуючий читач при видачі", [&]() {
        lib.borrowBook("U-999", "978-0131103627");
        });

    runScenario("Пошук неіснуючої книги за ISBN", [&]() {
        lib.findBook("000-0000000000");
        });

    runScenario("Пошук неіснуючої книги за назвою", [&]() {
        lib.findBookByTitle("Unknown Book Title");
        });

    runScenario("Усі примірники книги зайняті", [&]() {
        lib.borrowBook("U-101", "978-0201700732");
        });

    runScenario("Повернення простроченої книги", [&]() {
        lib.returnBook("U-101", "978-0262033848");
        });

    runScenario("Інша помилка бібліотеки (загальний LibraryException)", [&]() {
        lib.returnBook("U-102", "978-0131103627");
        });

    runScenario("Успішна видача доступної книги", [&]() {
        lib.borrowBook("U-101", "978-0131103627");
        });

    return 0;
}