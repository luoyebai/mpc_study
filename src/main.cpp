// controller
#include "controller/mpc.hpp"

int main() {
    const auto mpc_config_path =
        std::string(__CONFIG_DIR__) + "mpc_config.yaml";
    auto mpc_controller = controller::Mpc(mpc_config_path);

    mpc_controller.checkAllMatrix();
    mpc_controller.prepareAllVars();
    mpc_controller.display();
    std::cout << mpc_controller.traceTarget({400, -400}, {20, -20}) << "\n";
    return 0;
}
