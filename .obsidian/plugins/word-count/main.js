const { Plugin, MarkdownView } = require("obsidian");

module.exports = class WordCountPlugin extends Plugin {
  onload() {
    this.statusBar = this.addStatusBarItem();
    this.statusBar.setText("字数: ...");

    // 切换文件或编辑时更新
    this.registerEvent(
      this.app.workspace.on("active-leaf-change", () => this.refresh())
    );
    this.registerEvent(
      this.app.workspace.on("editor-change", () => this.refresh())
    );

    this.refresh();
  }

  refresh() {
    const view = this.app.workspace.getActiveViewOfType(MarkdownView);
    if (!view || !view.editor) {
      this.statusBar.setText("字数: --");
      return;
    }

    const text = view.editor.getValue();
    if (!text.trim()) {
      this.statusBar.setText("字数: 0 | 字符: 0 | 行: 0");
      return;
    }

    // 中文字数
    const cjk = (text.match(/[\u4e00-\u9fff\u3400-\u4dbf\u3000-\u303f\uff00-\uffef]/g) || []).length;
    // 英文/数字单词
    const noCjk = text.replace(/[\u4e00-\u9fff\u3400-\u4dbf\u3000-\u303f\uff00-\uffef]/g, " ");
    const words = noCjk.split(/\s+/).filter(w => /[a-zA-Z0-9]/.test(w)).length;
    // 总字符（无空白）
    const chars = text.replace(/\s/g, "").length;
    // 行数
    const lines = text.split(/\n/).length;

    this.statusBar.setText(`字数: ${cjk + words} | 字符: ${chars} | 行: ${lines}`);
  }

  onunload() {}
};
