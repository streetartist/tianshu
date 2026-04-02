import 'package:dio/dio.dart';

class ApiService {
  static final ApiService _instance = ApiService._();
  static ApiService get instance => _instance;

  late Dio _dio;
  String? _token;

  ApiService._() {
    _dio = Dio(BaseOptions(
      baseUrl: 'http://localhost:5000',
      connectTimeout: const Duration(seconds: 30),
    ));
  }

  void setToken(String? token) {
    _token = token;
  }

  Future<Response> get(String path) async {
    return _dio.get(path);
  }

  Future<Response> post(String path, {dynamic data}) async {
    return _dio.post(path, data: data);
  }
}
