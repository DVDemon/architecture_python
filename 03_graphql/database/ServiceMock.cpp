#include "ServiceMock.h"
#include <algorithm>
#include <atomic>
#include <mutex>
#include <vector>

namespace graphql::database::object
{

namespace {

struct UserStorage {
    std::vector<std::shared_ptr<UserImpl>> users;
    std::atomic<int> next_id{1};
    mutable std::mutex mutex;

    static UserStorage& instance() {
        static UserStorage storage;
        return storage;
    }

    std::string add_user(std::string first_name, std::string last_name, std::string email,
                        std::string title, std::string login, std::string password) {
        std::lock_guard<std::mutex> lock(mutex);
        auto user = std::make_shared<UserImpl>();
        user->_id = next_id++;
        user->_first_name = std::move(first_name);
        user->_last_name = std::move(last_name);
        user->_email = std::move(email);
        user->_title = std::move(title);
        user->_login = std::move(login);
        user->_password = std::move(password);
        users.push_back(user);
        return std::to_string(user->_id);
    }

    std::shared_ptr<User> get_user(int id) const {
        std::lock_guard<std::mutex> lock(mutex);
        auto it = std::find_if(users.begin(), users.end(),
            [id](const std::shared_ptr<UserImpl>& u) { return u->_id == id; });
        if (it == users.end())
            return nullptr;
        return std::make_shared<User>(*it);
    }

    std::vector<std::shared_ptr<User>> all_users() const {
        std::lock_guard<std::mutex> lock(mutex);
        std::vector<std::shared_ptr<User>> result;
        for (const auto& u : users)
            result.push_back(std::make_shared<User>(u));
        return result;
    }

    std::vector<std::shared_ptr<User>> search(const std::string& term1, const std::string& term2) const {
        std::lock_guard<std::mutex> lock(mutex);
        std::vector<std::shared_ptr<User>> result;
        for (const auto& u : users) {
            if (u->_first_name.find(term1) != std::string::npos &&
                u->_last_name.find(term2) != std::string::npos)
                result.push_back(std::make_shared<User>(u));
        }
        return result;
    }
};

} // anonymous namespace

    std::shared_ptr<graphql::service::Request> GetService()
    {
        std::shared_ptr<Query> query = std::make_shared<Query>(std::make_shared<QueryImpl>());
        std::shared_ptr<Mutations> mutation = std::make_shared<Mutations>(std::make_shared<MutationsImpl>());
        auto service = std::make_shared<Operations>(std::move(query), std::move(mutation));
        return service;
    }

    void QueryImpl::beginSelectionSet([[maybe_unused]] const service::SelectionSetParams &params) const
    {
    }
    void QueryImpl::endSelectionSet([[maybe_unused]] const service::SelectionSetParams &params) const
    {
    }

    std::shared_ptr<User> QueryImpl::getGet_user([[maybe_unused]] service::FieldParams&& params,
                                                    std::optional<int> &&idArg) const
    {
        if (!idArg.has_value())
            return nullptr;
        return UserStorage::instance().get_user(idArg.value());
    }

    std::optional<std::vector<std::shared_ptr<User>>> QueryImpl::getAll_users([[maybe_unused]] service::FieldParams&& params) const
    {
        return UserStorage::instance().all_users();
    }

    std::vector<std::shared_ptr<User>> QueryImpl::getSearch([[maybe_unused]] service::FieldParams&& params,
     std::string &&term1Arg, std::string &&term2Arg) const
    {
        return UserStorage::instance().search(term1Arg, term2Arg);
    }


    void MutationsImpl::beginSelectionSet([[maybe_unused]] const service::SelectionSetParams &params) const{

    }
    void MutationsImpl::endSelectionSet([[maybe_unused]] const service::SelectionSetParams &params) const{

    }
    std::string MutationsImpl::applyAdd_user([[maybe_unused]] service::FieldParams&& params,
                                              std::string && first_nameArg,
                                              std::string && last_nameArg,
                                              std::string && emailArg,
                                              std::string && titleArg,
                                              std::string && loginArg,
                                              std::string && passwordArg) const{

        return UserStorage::instance().add_user(
            std::move(first_nameArg),
            std::move(last_nameArg),
            std::move(emailArg),
            std::move(titleArg),
            std::move(loginArg),
            std::move(passwordArg)
        );
    }
}
