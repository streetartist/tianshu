/// 生成的文件模型
class GeneratedFile {
  final String path;
  final String content;

  const GeneratedFile({required this.path, required this.content});
}

/// 生成的项目
class GeneratedProject {
  final String name;
  final List<GeneratedFile> files;

  const GeneratedProject({required this.name, required this.files});

  /// 获取文件树结构
  Map<String, dynamic> get fileTree {
    final tree = <String, dynamic>{};
    for (final file in files) {
      final parts = file.path.split('/');
      var current = tree;
      for (var i = 0; i < parts.length - 1; i++) {
        current[parts[i]] ??= <String, dynamic>{};
        current = current[parts[i]] as Map<String, dynamic>;
      }
      current[parts.last] = file;
    }
    return tree;
  }
}
