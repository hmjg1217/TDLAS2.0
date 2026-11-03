#include "TemporaryCredentialAuthenticator.h"

int main()
{
    const authentication::TemporaryCredentialAuthenticator authenticator;

    if (authenticator.authenticate(QStringLiteral("00"), QStringLiteral("00"))
        != authentication::UserRole::Administrator) {
        return 1;
    }
    if (authenticator.authenticate(QStringLiteral("01"), QStringLiteral("01"))
        != authentication::UserRole::Client) {
        return 2;
    }
    if (authenticator.authenticate(QStringLiteral("00"), QStringLiteral("01"))
        != authentication::UserRole::Invalid) {
        return 3;
    }
    if (authenticator.authenticate(QStringLiteral("01"), QStringLiteral("00"))
        != authentication::UserRole::Invalid) {
        return 4;
    }
    if (authenticator.authenticate(QString(), QString())
        != authentication::UserRole::Invalid) {
        return 5;
    }
    if (authenticator.authenticate(QStringLiteral(" 00"), QStringLiteral("00"))
        != authentication::UserRole::Invalid) {
        return 6;
    }
    if (authenticator.authenticate(QStringLiteral("00"), QStringLiteral("00 "))
        != authentication::UserRole::Invalid) {
        return 7;
    }

    return 0;
}
