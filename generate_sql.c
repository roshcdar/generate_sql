#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TAG_LEN 2
#define LENGTH_LEN 2

typedef struct {
    char account_no[25];
    char avail_bal[13];
    char ledger_bal[13];
    char upd_date[9];
    char upd_time[7];
} Account;

void generate_sql(const Account* account) {
    printf("INSERT INTO T_ACCOUNT (ACCOUNT_NO, AVAIL_BAL, LEDGER_BAL, UPD_DATE, UPD_TIME) VALUES ('%s', %ld, ",
        account->account_no,
        atol(account->avail_bal + 1) * (account->avail_bal[0] == 'D' ? -1 : 1));

    if (strlen(account->ledger_bal) > 0) {
        // Insert ledger balance as a number
        printf("%ld", atol(account->ledger_bal + 1) * (account->ledger_bal[0] == 'D' ? -1 : 1));
    } else {
        // If ledger balance is not present, insert NULL
        printf("NULL");
    }

    // Continue with the rest of the fields
    printf(", %d, %d);\n",
        atoi(account->upd_date),
        atoi(account->upd_time));
}

bool is_complete(const Account* account)
{
    if (strlen(account->account_no) > 0 && strlen(account->avail_bal) > 0 &&
        strlen(account->upd_date) > 0 && strlen(account->upd_time) > 0)
    {
        return true;
    }
    return false;
}

void reset_account(Account* account) {
    memset(account, 0, sizeof(Account));
}

void write_complete_transaction(Account * account, const int commit_limit, int * commit_count)
{
    if (is_complete(account)) {
        generate_sql(account);
        if (commit_limit) // if commit limit is defined
        {
            ++(*commit_count);
            if (*commit_count >= commit_limit)
            {
                printf("COMMIT;\n");
                *commit_count = 0;
            }
        }
        // Prepare for the next account's records.
        reset_account(account);
    }
}

void parse_tag(const char *tag, int len, const char *value, Account *account, const int commit_limit, int * commit_count)
{
    if (strcmp(tag, "NA") == 0) {
            write_complete_transaction(account, commit_limit, commit_count);
            strncpy(account->account_no, value, len);
            if (len <= 24)
            {
                account->account_no[len] = '\0';
            }
            else
            {
                account->account_no[24] = '\0';
            }
        } else if (strcmp(tag, "AB") == 0) {
            strncpy(account->avail_bal, value, len);
            if (len <= 12)
            {
                account->avail_bal[len] = '\0';
            }
            else
            {
                account->avail_bal[12] = '\0';
            }
        } else if (strcmp(tag, "LB") == 0) {
            strncpy(account->ledger_bal, value, len);
            if (len <= 12)
            {
                account->ledger_bal[len] = '\0';
            }
            else
            {
                account->ledger_bal[12] = '\0';
            }
        } else if (strcmp(tag, "UT") == 0) {
             sscanf(value, "%4s-%2s-%2s %2s:%2s:%2s", 
                account->upd_date, account->upd_date + 4, account->upd_date + 6,
                account->upd_time, account->upd_time + 2, account->upd_time + 4);
        }
}

void parse(char** pos, Account* account, const int commit_limit, int * commit_count) {
    while (**pos) {
        char tag[TAG_LEN + 1] = {0};
        char len_str[LENGTH_LEN + 1] = {0};
        strncpy(tag, *pos, TAG_LEN);
        (*pos) += TAG_LEN;
        strncpy(len_str, *pos, LENGTH_LEN);
        (*pos) += LENGTH_LEN;
        int len = atoi(len_str);

        if (len < 0 || len > strlen(*pos)) {
            fprintf(stderr, "Invalid length encountered.\n");
            break;
        }

        // The value starts at 'pos' and spans 'len' characters
        parse_tag(tag, len, *pos, account, commit_limit, commit_count);
        
        // Move pos forward by the length of the value
        (*pos) += len;    
    }
}

int main(int argc, char *argv[]) {
    int commit_limit = 0, commit_count = 0;
    if (argc > 2 && strcmp(argv[1], "-c") == 0) {
        commit_limit = atoi(argv[2]);
    }

    char* buffer = NULL, *pos = NULL;
    size_t len = 0;
    Account account;
    reset_account(&account);

    if (getline(&buffer, &len, stdin) != -1)
    {
        pos = buffer;
    }
    else
    {
        free(buffer);
        return 0;
    }

    while (*pos || getline(&buffer, &len, stdin) != -1)  {
        if (!(*pos))
        {
            pos = buffer;
        }
        parse(&pos, &account, commit_limit, &commit_count);
    }
    write_complete_transaction(&account, commit_limit, &commit_count);
    
    if ((!commit_limit) || (commit_limit && commit_count > 0))
    {
        // Always commit at the end for data integrity
        printf("COMMIT;\n");
    }

    free(buffer); 
    return 0;
}