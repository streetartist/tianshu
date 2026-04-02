import 'dart:convert';
import 'dart:io';
import 'package:flutter/foundation.dart';
import 'package:file_picker/file_picker.dart';
import 'package:archive/archive.dart';
import 'package:universal_html/html.dart' as html;
import '../models/generated_project.dart';

/// 项目导出服务
class ProjectExporter {
  /// 导出项目
  static Future<bool> export(GeneratedProject project) async {
    if (kIsWeb) {
      return _exportAsZipWeb(project);
    } else {
      return _exportToFolder(project);
    }
  }

  /// 桌面端：选择文件夹并写入文件
  static Future<bool> _exportToFolder(GeneratedProject project) async {
    final result = await FilePicker.platform.getDirectoryPath(
      dialogTitle: '选择项目保存位置',
    );

    if (result == null) return false;

    final projectDir = Directory('$result/${project.name}');

    try {
      if (!await projectDir.exists()) {
        await projectDir.create(recursive: true);
      }

      for (final file in project.files) {
        final filePath = '${projectDir.path}/${file.path}';
        final fileObj = File(filePath);

        final parentDir = fileObj.parent;
        if (!await parentDir.exists()) {
          await parentDir.create(recursive: true);
        }

        await fileObj.writeAsString(file.content);
      }

      return true;
    } catch (e) {
      debugPrint('导出失败: $e');
      return false;
    }
  }

  /// Web端：生成ZIP并下载
  static Future<bool> _exportAsZipWeb(GeneratedProject project) async {
    try {
      final archive = Archive();

      for (final file in project.files) {
        final bytes = utf8.encode(file.content);
        archive.addFile(ArchiveFile(
          '${project.name}/${file.path}',
          bytes.length,
          bytes,
        ));
      }

      final zipData = ZipEncoder().encode(archive);
      if (zipData == null) return false;

      // 创建Blob并下载
      final blob = html.Blob([Uint8List.fromList(zipData)], 'application/zip');
      final url = html.Url.createObjectUrlFromBlob(blob);

      html.AnchorElement(href: url)
        ..setAttribute('download', '${project.name}.zip')
        ..click();

      html.Url.revokeObjectUrl(url);
      return true;
    } catch (e) {
      debugPrint('导出失败: $e');
      return false;
    }
  }
}
