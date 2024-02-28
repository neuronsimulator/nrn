#pragma once

struct TQItem {
    void clear(){};

    void* data_{};
    double t_{};
    TQItem* left_{};
    TQItem* right_{};
    TQItem* parent_{};
    int cnt_{};  // reused: -1 means it is in the splay tree, >=0 gives bin
};
