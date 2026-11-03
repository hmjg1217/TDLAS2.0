#pragma once

#include <QString>

namespace authentication {

enum class UserRole {
    Invalid,
    Administrator,
    Client
};

class TemporaryCredentialAuthenticator final
{
public:
    [[nodiscard]] UserRole authenticate(const QString &username, const QString &password) const;
};

} // namespace authentication
