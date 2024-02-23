#ifndef telegram_bot_h
#define telegram_bot_h

#include "Arduino.h"
#include <HTTPClient.h>
#include <UrlEncode.h>

class TelegramBot
{
  private:
    const char* _bot_token;
    const char* _chat_id;
    String _telegram_url;
    String _query;

  public:
    TelegramBot();

    const char* getBotToken();
    const char* getChatId();
    String getTelegramUrl();
    String getQuery();

    void setMessage(String message);
    void sendMessageToTelegram(String telegram_url, String query, String message);
};

#endif