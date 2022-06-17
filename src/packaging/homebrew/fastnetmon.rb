class Fastnetmon < Formula
  desc "DDoS detection tool with sFlow, Netflow, IPFIX and port mirror support"
  homepage "https://github.com/pavel-odintsov/fastnetmon/"
  head "https://github.com/pavel-odintsov/fastnetmon.git"
  license "GPL-2.0-only"
  revision 1

  depends_on "cmake" => :build
  depends_on "boost"
  depends_on "log4cpp"
  depends_on "hiredis"
  depends_on "json-c"
  depends_on "mongo-c-driver"
  depends_on "grpc"
  depends_on "capnp"
  depends_on "openssl@3"

  def install
    system "cmake", "-S", "src", "-B", "build", "-DENABLE_CUSTOM_BOOST_BUILD=FALSE", "-DDO_NOT_USE_SYSTEM_LIBRARIES_FOR_BUILD=FALSE", "-DCMAKE_SYSTEM_INCLUDE_PATH=/usr/local/opt/openssl\@3/include", "-DCMAKE_SYSTEM_LIBRARY_PATH=/usr/local/opt/openssl\@3/lib", *std_cmake_args
    system "cmake", "--build", "build"
    system "cmake", "--install", "build"
  end

 service do
    run [opt_sbin/"fastnetmon", "--configuration_file", etc/"fastnetmon.conf", "--log_to_console", "--disable_pid_logic"]
    keep_alive false
    working_dir HOMEBREW_PREFIX
  end
end

