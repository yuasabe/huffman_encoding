#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <assert.h>
#include <string.h>
#include "encode.h"

// ./huffman news.txt output
// で実行してください

// 各シンボルの数を数える為に
//このソースファイルでのみ有効なstatic な配列を設定
// 数を数える為に、1byteの上限+1で設定しておく
#define NSYMBOLS 256+1
static int symbol_count[NSYMBOLS];
// static int bit_length = 0;

typedef struct node {
  int symbol;
  int count;
  struct node *left;
  struct node *right;
} Node;

struct code {
  uint8_t code[NSYMBOLS];
  uint8_t length;
};

struct code code_list[NSYMBOLS];

static void count_symbols(const char *filename) {
  FILE *fp = fopen(filename, "rb");
  if (fp == NULL) {
    fprintf(stderr, "error: cannot open %s\n", filename);
    exit(1);
  }
  
  for (int i = 0; i < NSYMBOLS; i++) {
    symbol_count[i] = 0;
  }
  // 1Byteずつ読み込み、カウントする
  int ch;
  int text_length = 0;
  while (( ch = fgetc(fp)) != EOF ) {
    symbol_count[ch]++;
    text_length++;
  }
  // ファイル終端のみ特殊 (256) をカウント
  // 必ずしも必要ではないが
  symbol_count[NSYMBOLS-1]++; // End of File

  fclose(fp);

  for (int i = 0; i < NSYMBOLS; i++) {
    // printf("%c: %d\n", i, symbol_count[i]);
  }
}

static Node *pop_min(int *n, Node *nodep[]) {
  // Find the node with the smallest count
  // カウントが最小のノードを見つけてくる
  int argmin = 0;
  for (int i = 0; i < *n; i++) {
    if (nodep[i]->count < nodep[argmin]->count) {
      argmin = i;
    }
  }

  Node *node_min = nodep[argmin];

  // Remove the node pointer from nodep[]
  // 見つかったノード以降の配列を前につめていく
  for (int i = argmin; i < (*n) - 1; i++) {
    nodep[i] = nodep[i + 1];
  }
  // 合計ノード数を一つ減らす
  (*n)--;

  return node_min;
}

static Node *build_tree() {
  int n = 0;
  Node *nodep[NSYMBOLS];

  for (int i = 0; i < NSYMBOLS; i++) {
    if (symbol_count[i] == 0) continue;
    nodep[n] = malloc(sizeof(Node));
    nodep[n]->symbol = i;
    nodep[n]->count  = symbol_count[i];
    nodep[n]->left   = NULL;
    nodep[n]->right  = NULL;
    n++;
  }
  n--;

  // printf("n: %d\n",n);

  while (n >= 2) {
    Node *node1 = pop_min(&n, nodep);
    Node *node2 = pop_min(&n, nodep);
    // printf("extract %c and %c\n",node1->symbol,node2->symbol);

    // Create a new node
    // 選ばれた2つのノードを元に統合ノードを新規作成
    // 作成したノードはnodep にどうすればよいか?
    Node *node = malloc(sizeof(Node));
    node->left = node1;
    node->right = node2;
    node->symbol = '$';
    node->count = node1->count + node2->count;

    nodep[n] = node;
    n++;
  }
  for (int i = 0; nodep[i] != NULL; ++i) {
    // printf("%c %d\n",nodep[i]->symbol,nodep[i]->count);
  }
  // printf("%i %c\n",nodep[0]->left->right->right->count,nodep[0]->left->right->right->symbol);
  return nodep[0];
}

// Perform depth-first traversal of the tree
// 深さ優先で木を走査する
static void traverse_tree(const int depth, const Node *np) {
  static char code[8] = {0};

  assert(depth < NSYMBOLS);

  if (np->left == NULL) {
    code[depth] = '\0';
    if (np->symbol == '\n') {
      printf("EOF:%d\t: ", np->count);
    } else {
      printf("%c:%d\t: ", np->symbol, np->count);
    }
    printf("%s", code);
    printf("\n");
    memcpy(code_list[np->symbol].code, code, depth);
    code_list[np->symbol].length = depth;
    return;
  }

  code[depth] = '0';
  traverse_tree(depth + 1, np->left);
  code[depth] = '1';
  traverse_tree(depth + 1, np->right);
}

// この関数のみ外部 (main) で使用される (staticがついていない)
int encode(const char *input_file, const char *output_file) {
  count_symbols(input_file);
  Node *root = build_tree();
  traverse_tree(0, root);

  // encode each char of input_file and write it to output_file
  FILE *ifp = fopen(input_file, "rb");
  FILE *ofp;
  if (ifp == NULL) {
    fprintf(stderr, "error: cannot open input_file %s\n", input_file);
    exit(1);
  }
  if (!output_file) {
    ofp = stdout;
  } else {
    ofp = fopen(output_file, "wb");
  }
  uint8_t c2 = 0, used_bits = 0;
  int c = 0;
  // 入力ファイルから一文字ずつ読み込んで、コード符号が保存されてあるcode_listから適当な
  // コード符号を取り出して、ビット列を生成し出力ファイルに出力する
  while(!feof(ifp)) {
    c = fgetc(ifp);
    for (int i = 0; i < code_list[c].length; ++i) {
      // #define SETB(x, b) ((x) |= ((1) << (b)))
      if (code_list[c].code[i] == '1') {
        c2 |= (1) << (7 - used_bits);
      }
      used_bits++;
      if (used_bits == 8) {
        fputc(c2, ofp);
        used_bits = 0;
        c2 = 0;
      }
    }
  }
  if (used_bits) {
    fputc(c2, ofp);
  }
  fclose(ofp);

  // 入力ファイルをエンコードし、出力ファイルとして出力するところまではできました。
  // 実際にエンコードしたものをデコードするためには、この出力ファイルの先頭に
  // 各文字とそれに対応する符号を追記しておく必要があります。
  

  return 1;
}









