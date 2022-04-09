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

void TestAddedDocuments() {

    const int doc_id = 1;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3};

    const int doc_id_2 = 2;
    const string content_2 = "My dog is crazy"s;
    const vector<int> ratings_2 = {1, 2, 3};


    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        server.AddDocument(doc_id_2, content_2, DocumentStatus::ACTUAL, ratings_2);
        const auto found_docs = server.FindTopDocuments("cat"s);
        ASSERT_EQUAL(found_docs.size(), 1u);
    }

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

void TestMinusWord(){

    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3};

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

void TestGetStatusDoc(){

    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3};


    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);

        const auto found_docs = server.FindTopDocuments("cat", DocumentStatus::BANNED);

        ASSERT_EQUAL(found_docs.size(), 0);
    }

    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::BANNED, ratings);

        const auto found_docs = server.FindTopDocuments("cat", DocumentStatus::BANNED);

        ASSERT_EQUAL(found_docs.size(), 1);
    }

    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);

        const auto found_docs = server.FindTopDocuments("cat");

        ASSERT_EQUAL(found_docs.size(), 1);
    }
}

void TestGetRelevanceDocs(){

    const double EPSILON = 1e-6;

    const int doc_id = 1;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3};

    const int doc_id_2 = 2;
    const string content_2 = "dog white and black"s;
    const vector<int> ratings_2 = {2, 3, 6};

    {

        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        server.AddDocument(doc_id_2, content_2, DocumentStatus::ACTUAL, ratings_2);
        const auto found_docs = server.FindTopDocuments("cat");
        ASSERT(found_docs[0].relevance -  log( server.GetDocumentCount() * 1.0 / 1 ) * ( 1. / 4) < EPSILON);

    }

    {

        const int doc_id_3 = 3;
        const string content_3 = "My cat is crazy he play evryday!! "s;
        const vector<int> ratings_3 = {2, 3, 6};

        const int doc_id_4 = 4;
        const string content_4 = "Lie cat"s;
        const vector<int> ratings_4 = {2, 3, 6};

        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        server.AddDocument(doc_id_2, content_2, DocumentStatus::ACTUAL, ratings_2);
        server.AddDocument(doc_id_3, content_3, DocumentStatus::ACTUAL, ratings_3);
        server.AddDocument(doc_id_4, content_4, DocumentStatus::ACTUAL, ratings_4);
        const auto found_docs = server.FindTopDocuments("cat");
        ASSERT_EQUAL( found_docs.size(), 3 );
        ASSERT( found_docs[0].relevance > found_docs[1].relevance and found_docs[1].relevance > found_docs[2].relevance);

    }
}

void TestGetRatingDocs(){

    const int doc_id = 1;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 5, -10};

    const int doc_id_2 = 3;
    const string content_2 = "cat white and cat black"s;
    const vector<int> ratings_2 = {};

    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("cat");

        ASSERT_EQUAL(found_docs[0].rating, ( 1 + 5 -10 ) / 3 );
    }

    {
        SearchServer server;
        server.AddDocument(doc_id_2, content_2, DocumentStatus::ACTUAL, ratings_2);
        const auto found_docs = server.FindTopDocuments("cat");

        ASSERT_EQUAL(found_docs[0].rating, 0);
    }
}

void TestFunctionalPredictFunction(){

    const int doc_id = 1;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3};

    const int doc_id_2 = 4;
    const string content_2 = "cat in the village"s;
    const vector<int> ratings_2 = {2, 3, 6};

    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        server.AddDocument(doc_id_2, content_2, DocumentStatus::BANNED, ratings_2);
        const auto found_docs = server.FindTopDocuments("cat", [](int doc_id, DocumentStatus status, int) { return doc_id % 2 == 0 and status == DocumentStatus::BANNED; });
        ASSERT_EQUAL(found_docs.size(), 1);
        ASSERT_EQUAL(found_docs[0].id, 4);

    }

    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        server.AddDocument(doc_id_2, content_2, DocumentStatus::BANNED, ratings_2);
        const auto found_docs = server.FindTopDocuments("cat", [](int doc_id, DocumentStatus, int rating) { return doc_id < 5 and rating >= 2; });
        ASSERT_EQUAL(found_docs.size(), 2);
    }
}

void TestFunctionalMatchDocument(){


    const int doc_id = 1;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3};

    {

        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto [vector_word, DocumentStatus ] = server.MatchDocument("cat the black", 1);

        ASSERT_EQUAL(vector_word.size(), 2 );

    }

    {


        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto [vector_word, DocumentStatus ] = server.MatchDocument("cat the -city", 1);
        ASSERT(vector_word.empty());
    }

}

void TestSearchServer() {

    RUN_TEST(TestAddedDocuments);
    RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
    RUN_TEST(TestMinusWord);
    RUN_TEST(TestGetStatusDoc);
    RUN_TEST(TestFunctionalPredictFunction);
    RUN_TEST(TestGetRelevanceDocs);
    RUN_TEST(TestGetRatingDocs);
    RUN_TEST(TestFunctionalMatchDocument);
}

