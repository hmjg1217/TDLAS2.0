#include "TemporaryCredentialAuthenticator.h"

namespace authentication {

UserRole TemporaryCredentialAuthenticator::authenticate(
    const QString &username,
    const QString &password) const
{
    if (username == QStringLiteral("00") && password == QStringLiteral("00")) {
        return UserRole::Administrator;
    }
    if (username == QStringLiteral("01") && password == QStringLiteral("01")) {
        return UserRole::Client;
    }
    return UserRole::Invalid;
}

} // namespace authentication
