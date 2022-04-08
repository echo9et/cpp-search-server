template<typename U, typename T>
void AssertEqualImpl(const U& l, const T& r, const string& l_srt, const string& r_str, const string& file, const string& func, unsigned line, const string& hint = ""){

    if (l != r) {
            cerr << boolalpha;
            cerr << file << "("s << line << "): "s << func << ": "s;
            cerr << "ASSERT_EQUAL("s << l_srt << ", "s << r_str << ") failed: "s;
            cerr << l << " != "s << r << "."s;
            if (!hint.empty()) {
                cerr << " Hint: "s << hint;
            }
            cerr << endl;
            abort();
        }

}

//#define ASSERT_EQUAL( a, b ) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__)
//#define ASSERT_EQUAL_HINT( a, b, hint ) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, #hint)

template<typename T>
void AssertImpl(const T& value, const string& value_str, const string& file, const string& func, unsigned line, const string& hint = ""){

    if (!value) {
        cerr << file << "("s << line << "): "s << func << ": "s;
        cerr << "ASSERT("s << value_str << ") failed."s;
        if (!hint.empty()) {
            cerr << " Hint: "s << hint;
        }
        cerr << endl;
        abort();
    }

}
//#define ASSERT( a ) AssertImpl(!!(a), #a, __FILE__, __FUNCTION__, __LINE__)
//#define ASSERT_HINT( a, hint ) AssertImpl(!!(a), #a, __FILE__, __FUNCTION__, __LINE__, #hint)

template <typename f>
void RunTestImpl(const f& func, const string& func_name) {
    func();
    cerr << func_name << " OK" << endl;
}

void TestExcludeStopWordsFromAddedDocumentContent() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3};
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in"s);
        ASSERT_EQUAL(found_docs.size(), 1u);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);
    }

    {
        SearchServer server;
        server.SetStopWords("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT_HINT(server.FindTopDocuments("in"s).empty(), "Stop words must be excluded from documents"s);
    }
}
void TestExcludeMinusWord(){

    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3};
    // Сначала убеждаемся, что поиск слова, не входящего в список стоп-слов,
    // находит нужный документ
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in -city"s);
        ASSERT_EQUAL(found_docs.size(), 0);
    }

    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("-city"s);
        ASSERT_EQUAL(found_docs.size(),  0);
    }

    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments(""s);
        ASSERT_EQUAL(found_docs.size(), 0);
    }

    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments(" "s);
        ASSERT_EQUAL(found_docs.size(), 0);
    }

    const int doc_id_2 = 4;
    const string content_2 = "cat in the village"s;
    const vector<int> ratings_2 = {2, 3, 6};


    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        server.AddDocument(doc_id_2, content_2, DocumentStatus::ACTUAL, ratings_2);
        const auto found_docs = server.FindTopDocuments("in -city"s);
        ASSERT_EQUAL(found_docs.size(), 1);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id_2);
    }
}

void TestExcludeGetStatusDoc(){

    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3};

    const int doc_id_2 = 4;
    const string content_2 = "cat in the village"s;
    const vector<int> ratings_2 = {2, 3, 6};

    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        server.AddDocument(doc_id_2, content_2, DocumentStatus::BANNED, ratings_2);

        const auto found_docs = server.FindTopDocuments("cat", [](int, DocumentStatus s, int) { return s == DocumentStatus::BANNED; });

        ASSERT_EQUAL(found_docs.size(), 1);
    }

    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::BANNED, ratings);
        server.AddDocument(doc_id_2, content_2, DocumentStatus::BANNED, ratings_2);

        const auto found_docs = server.FindTopDocuments("cat", [](int, DocumentStatus s, int) { return s == DocumentStatus::BANNED; });

        ASSERT_EQUAL(found_docs.size(), 2);
    }

    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::BANNED, ratings);
        server.AddDocument(doc_id_2, content_2, DocumentStatus::BANNED, ratings_2);

        const auto found_docs = server.FindTopDocuments("cat", [](int, DocumentStatus s, int) { return s == DocumentStatus::ACTUAL; });

        ASSERT_EQUAL(found_docs.size(), 0);
    }

    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        server.AddDocument(doc_id_2, content_2, DocumentStatus::ACTUAL, ratings_2);

        const auto found_docs = server.FindTopDocuments("cat");

        ASSERT_EQUAL(found_docs.size(), 2);
    }

    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        server.AddDocument(doc_id_2, content_2, DocumentStatus::BANNED, ratings_2);

        const auto found_docs = server.FindTopDocuments("cat");

        ASSERT_EQUAL(found_docs.size(), 1);
    }
}

void TestExcludeRelevanceDocs(){

    const double EPSILON = 1e-6;

    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3};

    const int doc_id_2 = 5;
    const string content_2 = "cat white and cat black"s;
    const vector<int> ratings_2 = {2, 3, 6};


    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("cat");
        ASSERT(found_docs[0].relevance - 0.162186 < EPSILON);


    }

    {
        SearchServer server;
        server.AddDocument(doc_id_2, content_2, DocumentStatus::ACTUAL, ratings_2);
        const auto found_docs = server.FindTopDocuments("cat");
        ASSERT(found_docs[0].relevance - 0.101366 < EPSILON);
    }


}

void TestExcludeRatingDocs(){

    const int doc_id = 1;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 5, -10};

    const int doc_id_2 = 2;
    const string content_2 = "cat white and cat black"s;
    const vector<int> ratings_2 = {2, 4, 6};

    const int doc_id_3 = 3;
    const string content_3 = "cat white and cat black"s;
    const vector<int> ratings_3 = {5};

    const int doc_id_4 = 4;
    const string content_4 = "cat white and cat black"s;
    const vector<int> ratings_4 = {};

    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("cat");

        ASSERT_EQUAL(found_docs[0].rating, -1);
    }
    {
        SearchServer server;
        server.AddDocument(doc_id_2, content_2, DocumentStatus::ACTUAL, ratings_2);
        const auto found_docs = server.FindTopDocuments("cat");

        ASSERT_EQUAL(found_docs[0].rating, 4);
    }

    {
        SearchServer server;
        server.AddDocument(doc_id_3, content_3, DocumentStatus::ACTUAL, ratings_3);
        const auto found_docs = server.FindTopDocuments("cat");

        ASSERT_EQUAL(found_docs[0].rating, 5);
    }

    {
        SearchServer server;
        server.AddDocument(doc_id_4, content_4, DocumentStatus::ACTUAL, ratings_4);
        const auto found_docs = server.FindTopDocuments("cat");

        ASSERT_EQUAL(found_docs[0].rating, 0);
    }
}

void TestSearchServer() {
    RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
    RUN_TEST(TestExcludeMinusWord);
    RUN_TEST(TestExcludeGetStatusDoc);
    RUN_TEST(TestExcludeRelevanceDocs);
    RUN_TEST(TestExcludeRatingDocs);
    // Не забудьте вызывать остальные тесты здесь
}
