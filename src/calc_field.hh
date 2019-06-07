#include "Config.hh"
#include "rvlm/core/SolidArray3d.hh"
#include "yee_grid.hh"

template <typename valueType>
void calcD(rvlm::core::SolidArray3d<valueType>& D,
           valueType deltaT,
           rvlm::core::SolidArray3d<valueType> const& perm,
           rvlm::core::SolidArray3d<valueType> const& sigma) {

    for (int ix = 0; ix < D.getCountX(); ix++)
    for (int iy = 0; iy < D.getCountY(); iy++)
    for (int iz = 0; iz < D.getCountZ(); iz++) {
        valueType& curD     = D.at(ix, iy, iz);
        valueType  curPerm  = perm.at(ix, iy, iz);
        valueType  curSigma = sigma.at(ix, iy, iz);
        valueType  subexpr  = deltaT / curPerm;
        curD = subexpr / (1 + curSigma * subexpr/2);
    }
}

template <typename valueType>
void calcC(rvlm::core::SolidArray3d<valueType>& C,
           valueType deltaT,
           rvlm::core::SolidArray3d<valueType> const& perm,
           rvlm::core::SolidArray3d<valueType> const& sigma) {

    for (int ix = 0; ix < C.getCountX(); ix++)
    for (int iy = 0; iy < C.getCountY(); iy++)
    for (int iz = 0; iz < C.getCountZ(); iz++) {
        valueType& curC     = C.at(ix, iy, iz);
        valueType  curPerm  = perm.at(ix, iy, iz);
        valueType  curSigma = sigma.at(ix, iy, iz);
        valueType  subexpr  = curSigma * deltaT / (2 * curPerm);
        curC = (1 - subexpr) / (1 + subexpr);
    }
}

template <typename valueType>
void calcCoefs(YeeGrid<valueType>& grid) {

    calcD(grid.D_Hx, grid.delta_t, grid.mu_Hx, grid.sigma_Hx);
    calcD(grid.D_Hy, grid.delta_t, grid.mu_Hy, grid.sigma_Hy);
    calcD(grid.D_Hz, grid.delta_t, grid.mu_Hz, grid.sigma_Hz);

    calcC(grid.C_Ex, grid.delta_t, grid.epsilon_Ex, grid.sigma_Ex);
    calcC(grid.C_Ey, grid.delta_t, grid.epsilon_Ey, grid.sigma_Ey);
    calcC(grid.C_Ez, grid.delta_t, grid.epsilon_Ez, grid.sigma_Ez);

    calcD(grid.D_Ex, grid.delta_t, grid.epsilon_Ex, grid.sigma_Ex);
    calcD(grid.D_Ey, grid.delta_t, grid.epsilon_Ey, grid.sigma_Ey);
    calcD(grid.D_Ez, grid.delta_t, grid.epsilon_Ez, grid.sigma_Ez);
}

template <bool useKahan, typename valueType>
void calcH(YeeGrid<valueType>& grid) {

    int nx = grid.Hx.getCountX();
    int ny = grid.Hx.getCountY();
    int nz = grid.Hx.getCountZ();

    valueType delta_x = grid.delta_x;
    valueType delta_y = grid.delta_y;
    valueType delta_z = grid.delta_z;

    for(int ix = 0; ix < nx-1; ix++)
    for(int iy = 0; iy < ny-1; iy++)
    for(int iz = 0; iz < nz-1; iz++) {
        valueType& curHx      = grid.Hx          .at(ix,   iy,   iz);
        valueType  curD_Hx    = grid.D_Hx        .at(ix,   iy,   iz);
        valueType  curEz1     = grid.Ez          .at(ix,   iy+1, iz);
        valueType  curEz0     = grid.Ez          .at(ix,   iy,   iz);
        valueType  curEy1     = grid.Ey          .at(ix,   iy,   iz+1);
        valueType  curEy0     = grid.Ey          .at(ix,   iy,   iz);

        if constexpr (!useKahan) {
        curHx += curD_Hx * (
                    (curEy1 - curEy0) / delta_z -
                    (curEz1 - curEz0) / delta_y
                 );
        } else {
            valueType x = curD_Hx * (
                    (curEy1 - curEy0) / delta_z -
                    (curEz1 - curEz0) / delta_y
                 );

            valueType& sum = curHx;
            valueType& c = grid.c_Hx.at(ix, iy, iz);
            valueType y = x - c;
            valueType volatile t = sum + y;
            c = (t - sum) - y;
            sum = t;
        }
    }

    for(int ix = 0; ix < nx-1; ix++)
    for(int iy = 0; iy < ny-1; iy++)
    for(int iz = 0; iz < nz-1; iz++) {
        valueType& curHy      = grid.Hy          .at(ix,   iy,   iz);
        valueType  curD_Hy    = grid.D_Hy        .at(ix,   iy,   iz);
        valueType  curEx1     = grid.Ex          .at(ix,   iy,   iz+1);
        valueType  curEx0     = grid.Ex          .at(ix,   iy,   iz);
        valueType  curEz1     = grid.Ez          .at(ix+1, iy,   iz);
        valueType  curEz0     = grid.Ez          .at(ix,   iy,   iz);

        if constexpr (!useKahan) {
        curHy += curD_Hy * (
                    (curEz1 - curEz0) / delta_x -
                    (curEx1 - curEx0) / delta_z
                  );
        } else {
            valueType x = curD_Hy * (
                    (curEz1 - curEz0) / delta_x -
                    (curEx1 - curEx0) / delta_z
                  );


            valueType& sum = curHy;
            valueType& c = grid.c_Hy.at(ix, iy, iz);
            valueType y = x - c;
            valueType volatile t = sum + y;
            c = (t - sum) - y;
            sum = t;
        }
    }

    for(int ix = 0; ix < nx-1; ix++)
    for(int iy = 0; iy < ny-1; iy++)
    for(int iz = 0; iz < nz-1; iz++) {
        valueType& curHz      = grid.Hz          .at(ix,   iy,   iz);
        valueType  curD_Hz    = grid.D_Hz        .at(ix,   iy,   iz);
        valueType  curEy1     = grid.Ey          .at(ix+1, iy,   iz);
        valueType  curEy0     = grid.Ey          .at(ix,   iy,   iz);
        valueType  curEx1     = grid.Ex          .at(ix,   iy+1, iz);
        valueType  curEx0     = grid.Ex          .at(ix,   iy,   iz);

        if constexpr (!useKahan) {
        curHz += curD_Hz * (
                            (curEx1 - curEx0) / delta_y -
                            (curEy1 - curEy0) / delta_x
                    );
        } else {
            valueType x = curD_Hz * (
                            (curEx1 - curEx0) / delta_y -
                            (curEy1 - curEy0) / delta_x
                    );

            valueType& sum = curHz;
            valueType& c = grid.c_Hz.at(ix, iy, iz);
            valueType y = x - c;
            valueType volatile t = sum + y;
            c = (t - sum) - y;
            sum = t;
        }
    }
}

template <typename valueType>
void calcE(YeeGrid<valueType>& grid) {

    int nx = grid.Ex.getCountX();
    int ny = grid.Ex.getCountY();
    int nz = grid.Ex.getCountZ();

    valueType delta_x = grid.delta_x;
    valueType delta_y = grid.delta_y;
    valueType delta_z = grid.delta_z;

    for(int ix = 1; ix < nx; ix++)
    for(int iy = 1; iy < ny; iy++)
    for(int iz = 1; iz < nz; iz++) {
        valueType& curEx      = grid.Ex          .at(ix,   iy,   iz);
        valueType  curC_Ex    = grid.C_Ex        .at(ix,   iy,   iz);
        valueType  curD_Ex    = grid.D_Ex        .at(ix,   iy,   iz);
        valueType  curHz0     = grid.Hz          .at(ix,   iy,   iz);
        valueType  curHz1     = grid.Hz          .at(ix,   iy-1, iz);
        valueType  curHy0     = grid.Hy          .at(ix,   iy,   iz);
        valueType  curHy1     = grid.Hy          .at(ix,   iy,   iz-1);

        curEx = curC_Ex * curEx + curD_Ex * ((curHz0 - curHz1) / delta_y -
                                             (curHy0 - curHy1) / delta_z);
    }

    for(int ix = 1; ix < nx; ix++)
    for(int iy = 1; iy < ny; iy++)
    for(int iz = 1; iz < nz; iz++) {
        valueType& curEy      = grid.Ey          .at(ix,   iy,   iz);
        valueType  curC_Ey    = grid.C_Ey        .at(ix,   iy,   iz);
        valueType  curD_Ey    = grid.D_Ey        .at(ix,   iy,   iz);
        valueType  curHx0     = grid.Hx          .at(ix,   iy,   iz);
        valueType  curHx1     = grid.Hx          .at(ix,   iy,   iz-1);
        valueType  curHz0     = grid.Hz          .at(ix,   iy,   iz);
        valueType  curHz1     = grid.Hz          .at(ix-1, iy,   iz);

        curEy = curC_Ey * curEy + curD_Ey * ((curHx0 - curHx1) / delta_z -
                                             (curHz0 - curHz1) / delta_x);
    }

    for(int ix = 1; ix < nx; ix++)
    for(int iy = 1; iy < ny; iy++)
    for(int iz = 1; iz < nz; iz++) {
        valueType& curEz      = grid.Ez          .at(ix,   iy,   iz);
        valueType  curC_Ez    = grid.C_Ez        .at(ix,   iy,   iz);
        valueType  curD_Ez    = grid.D_Ez        .at(ix,   iy,   iz);
        valueType  curHy0     = grid.Hy          .at(ix,   iy,   iz);
        valueType  curHy1     = grid.Hy          .at(ix-1, iy,   iz);
        valueType  curHx0     = grid.Hx          .at(ix,   iy,   iz);
        valueType  curHx1     = grid.Hx          .at(ix,   iy-1, iz);

        curEz = curC_Ez * curEz + curD_Ez * ((curHy0 - curHy1) / delta_x -
                                             (curHx0 - curHx1) / delta_y);
    }
}
