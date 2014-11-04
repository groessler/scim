/* 
----------------------------------------------------------------------------------------
El UNDO y REDO funciona con una lista de estructuras de tipo 'struct undo'.
Las mismas contienen
      p_ant: puntero a otra estructura de tipo 'struct undo'. Si es NULL, indica que este nodo
             representa el primer cambio luego de cargada la planilla.
      struct ent * added: lista de ents agregados por el cambio
      struct ent * removed: lista de ents eliminados por el cambio
      struct undo_range_shift * range_shift: un rango que sufre un desplazamiento por el cambio
      p_sig: puntero a otra estructura de tipo 'struct undo'. Si es NULL, indica que este nodo
             representa el último cambio, y no hay ningun cambio posterior.

Siguiendo un esquema de UNDO / REDO habitual, si se realiza algún cambio (C1), se hace UNDO
y si luego se realiza otro cambio (C2) a partir de dicha posición, todas las acciones
posteriores se eliminan.
Esquema:
 
+ C1 -> + -> UNDO -
^                 \
|_                |
  \---------------/
|
|
|
 \-> C2 --> + ...

La estructura undo_shift_range contiene:
    int delta_rows: delta de filas por el que se desplaza el rango
    int delta_cols: delta de columnas por el que se desplaza el rango
    int tlrow:      fila superior izquierda que define el rango sobre el que se hara el desplazamiento
                    (tal cual se hiciera shift de un rango de celdas)
    int tlcol:      columna superior izquierda que define el rango sobre el que se hara el desplazamiento
                    (tal cual se hiciera shift de un rango de celdas)
    int brrow:      fila inferior derecha que define el rango sobre el que se hara el desplazamiento
                    (tal cual se hiciera shift de un rango de celdas)
    int brcol:      columna inferior derecha que define el rango sobre el que se hara el desplazamiento
                    (tal cual se hiciera shift de un rango de celdas)

Acciones cuyo UNDO/REDO se encuentra implementado:
1. eliminación de contenido de celda o un rango
2. ingreso de contenido en una celda en puntual
3. edición de una celda en puntual
4. justificado de una celda en puntual
5. paste de rango o celda
6. shift de rango o celda con sh sj sk sl
7. insert de row o columna
8. delete de row o columna
9. paste de row o columna

NO implementados:
1. cambio de formato de columna (en un futuro de celdas)
2. cambio de justificado de celdas

----------------------------------------------------------------------------------------
*/

#include "undo.h"
#include "macros.h"
#include "curses.h"
#include "sc.h"
#include <stdlib.h>


// undolist
static struct undo * undo_list = NULL;

// posición actual en lista
static int undo_list_pos = 0;

// cantidad de elementos en la lista
static int undo_list_len = 0;

// variable temporal
static struct undo undo_item;




// setea en blanco el undo_item
void create_undo_action() {
    undo_item.added       = NULL;
    undo_item.removed     = NULL;
    undo_item.p_ant       = NULL;
    undo_item.p_sig       = NULL;
    undo_item.p_sig       = NULL;
    undo_item.range_shift = NULL;
    return;
}

// esta función guarda en la undolist, una copia del
// undo_item con los datos de ents agregados o quitados
// y de undo range shift struct
void end_undo_action() {
    add_to_undolist(undo_item);
    return;
}

// Función que agrega un nodo undo a la lista undolist,
// hace el malloc del undo struct, completa la variable
// con el valor de la variable estatica undo_item
// y lo agrega a la lista
void add_to_undolist(struct undo u) {
        // Si no estamos al final de lista, borro desde la posicion al final.
        if ( undo_list != NULL && undo_list_pos != len_undo_list() )
            clear_from_here();

        struct undo * ul = (struct undo *) malloc (sizeof(struct undo));
        ul->p_sig = NULL;

        // Agrego ents
        ul->added = u.added;
        ul->removed = u.removed;
        ul->range_shift = u.range_shift;

        if (undo_list == NULL) {
            ul->p_ant = NULL;
            undo_list = ul;
        } else {
            ul->p_ant = undo_list;

            // Voy hasta el final de la lista
            while (undo_list->p_sig != NULL) undo_list = undo_list->p_sig;

            undo_list->p_sig = ul;
            undo_list = undo_list->p_sig;
        }
        undo_list_pos++;
        undo_list_len++;
    return;
}

// Funcion que libera le memoria de un nodo UNDO
// y los subsiguientes que tenga enlazados
void free_undo_node(struct undo * ul) {

    struct ent * de;
    struct ent * en;
    struct undo * e;

    // Borro desde posicion actual en adelante
    while (ul != NULL) {
        en = ul->added;
        while (en != NULL) {
            de = en->next;
            clearent(en);
            free(en);
            en = de;
        }
        en = ul->removed;
        while (en != NULL) {
            de = en->next;
            clearent(en);
            free(en);
            en = de;
        }
        e = ul->p_sig;
        if (ul->range_shift != NULL) free(ul->range_shift); // libero memoria de undo_range_shift 
        free(ul);
        undo_list_len--;
        ul = e;
    }
    return;
}

// Función que elimina de la lista undolist, los nodos que se encuentran
// a partir de la posicion actual
void clear_from_here() {
    if (undo_list == NULL) return;

    if (undo_list->p_ant == NULL) {
        free_undo_node(undo_list); 
        undo_list = NULL;
    } else {
        struct undo * ul = undo_list->p_sig; // ANTERIOR
        free_undo_node(ul); 
        undo_list->p_sig = NULL;
    }

    return;
}

// Funcion que elimina todo contenido de la UNDOLIST
void clear_undo_list () {
    if (undo_list == NULL) return;

    // Voy al comienzo de lista
    while (undo_list->p_ant != NULL ) {
        undo_list = undo_list->p_ant;
    }

    struct undo * ul = undo_list;
    struct undo * e;

    free_undo_node(ul); 

    undo_list = NULL;
    undo_list_pos = 0;
    //undo_list_len = 0; // no es necesario ya que se decrementa en free_undo_node.

    return;
}

int len_undo_list() {
    return undo_list_len;
/*
    struct undo * ul = undo_list;
    int c = 0;
    if (ul == NULL) return c;

    // Voy al comienzo de lista
    while (ul != NULL &&
    ul->p_ant != NULL ) {
        ul = ul->p_ant;
    }

    while (ul != NULL) {
        ul = ul->p_sig;
        c++;
    }

    return c;
*/
}

// esta función toma un rango de ents,
// y crea nuevos ents (tantos, como ents dentro del rango especificado).
// luego copia en los nuevos ents el contenido de los originales
// y los guarda en la lista added o removed del undo_item, segun el char type.
void copy_to_undostruct (int row_desde, int col_desde, int row_hasta, int col_hasta, char type) {
    int c, r;
    for (r = row_desde; r <= row_hasta; r++)
        for (c = col_desde; c <= col_hasta; c++) {            
            struct ent * e = (struct ent *) malloc( (unsigned) sizeof(struct ent) );
            cleanent(e);
            copyent(e, lookat(r, c), 0, 0, 0, 0, r, c, 0);

            // agrego ent al comienzo
            if (type == 'a') {
                e->next = undo_item.added;
                undo_item.added = e;
            } else {
                e->next = undo_item.removed;
                undo_item.removed = e;
            }

        }
    return;
}

// esta funcion toma un rango y un delta de filas y columnas y lo guarda en la struct de undo
// se utiliza para cuando se hace UNDO o REDO se desplace un rango sin necesidad de duplicar
// todos los ents de ese rango cuando solo se modifica su ubicación.
void save_undo_range_shift(int delta_rows, int delta_cols, int tlrow, int tlcol, int brrow, int brcol) {
    struct undo_range_shift * urs = (struct undo_range_shift *) malloc( (unsigned) sizeof(struct undo_range_shift ) );
    urs->delta_rows = delta_rows;
    urs->delta_cols = delta_cols;
    urs->tlrow = tlrow;
    urs->tlcol = tlcol;
    urs->brrow = brrow;
    urs->brcol = brcol;
    undo_item.range_shift = urs;
    return;
}   

// Funcion que realiza un UNDO
// En esta función se desplaza un rango de undo shift range a la posición original, en caso de existir,
// se agregan los ent de removed y se borran los de added
void do_undo() {
    if (undo_list == NULL || undo_list_pos == 0) {
        error("Not UNDO's left");
        return;
    }
    //info("%d %d", undo_list_pos, len_undo_list());

    int ori_currow = currow;
    int ori_curcol = curcol;

    struct undo * ul = undo_list;

    // realizo el undo shift range, en caso de existir
    if (ul->range_shift != NULL) {
        // fix marks
        if (ul->range_shift->delta_rows > 0)      // sj
            fix_marks(-(ul->range_shift->brrow - ul->range_shift->tlrow + 1), 0, ul->range_shift->tlrow, maxrow, ul->range_shift->tlcol, ul->range_shift->brcol);
        else if (ul->range_shift->delta_rows < 0) // sk
            fix_marks( (ul->range_shift->brrow - ul->range_shift->tlrow + 1), 0, ul->range_shift->tlrow, maxrow, ul->range_shift->tlcol, ul->range_shift->brcol);
        if (ul->range_shift->delta_cols > 0)      // sl
            fix_marks(0, -(ul->range_shift->brcol - ul->range_shift->tlcol + 1), ul->range_shift->tlrow, ul->range_shift->brrow, ul->range_shift->tlcol, maxcol);
        else if (ul->range_shift->delta_cols < 0) // sh
            fix_marks(0,  (ul->range_shift->brcol - ul->range_shift->tlcol + 1), ul->range_shift->tlrow, ul->range_shift->brrow, ul->range_shift->tlcol, maxcol);

        shift_range(- ul->range_shift->delta_rows, - ul->range_shift->delta_cols,
                      ul->range_shift->tlrow, ul->range_shift->tlcol, ul->range_shift->brrow, ul->range_shift->brcol);
    }

    // Borro los ent de added
    struct ent * i = ul->added;
    while (i != NULL) {
        struct ent * pp = *ATBL(tbl, i->row, i->col);
        clearent(pp);
        cleanent(pp);
        i = i->next;
    }
  
    // Cambio foco
    //if (ul->removed != NULL) {
    //    currow = ul->removed->row;
    //    curcol = ul->removed->col;
    //}

    // Agrego los ent de removed.
    struct ent * j = ul->removed;
    while (j != NULL) {
        struct ent * e_now = lookat(j->row, j->col);
        (void) copyent(e_now, j, 0, 0, 0, 0, j->row, j->col, 0);
        j = j->next;
    }
    
    currow = ori_currow;
    curcol = ori_curcol;

    if (undo_list->p_ant != NULL) undo_list = undo_list->p_ant;
    undo_list_pos--;
    info("Change: %d of %d", undo_list_pos, len_undo_list());

    return;
}

// Función que realiza un REDO
// En esta función se desplaza un rango de undo shift range en caso de existir,
// se agregan los ent de added y se borran los de removed
void do_redo() {
    if ( undo_list == NULL || undo_list_pos == len_undo_list()  ) {
        error("Not REDO's left");
        return;
    }
    //info("%d %d", undo_list_pos, len_undo_list());

    int ori_currow = currow;
    int ori_curcol = curcol;

    if (undo_list->p_ant == NULL && undo_list_pos == 0) ;
    else if (undo_list->p_sig != NULL) undo_list = undo_list->p_sig;

    struct undo * ul = undo_list;

    // realizo el undo shift range, en caso de existir
    if (ul->range_shift != NULL) {
        // fix marks
        if (ul->range_shift->delta_rows > 0)      // sj
            fix_marks( (ul->range_shift->brrow - ul->range_shift->tlrow + 1), 0, ul->range_shift->tlrow, maxrow, ul->range_shift->tlcol, ul->range_shift->brcol);
        else if (ul->range_shift->delta_rows < 0) // sk
            fix_marks(-(ul->range_shift->brrow - ul->range_shift->tlrow + 1), 0, ul->range_shift->tlrow, maxrow, ul->range_shift->tlcol, ul->range_shift->brcol);
        if (ul->range_shift->delta_cols > 0)      // sl
            fix_marks(0,  (ul->range_shift->brcol - ul->range_shift->tlcol + 1), ul->range_shift->tlrow, ul->range_shift->brrow, ul->range_shift->tlcol, maxcol);
        else if (ul->range_shift->delta_cols < 0) // sh
            fix_marks(0, -(ul->range_shift->brcol - ul->range_shift->tlcol + 1), ul->range_shift->tlrow, ul->range_shift->brrow, ul->range_shift->tlcol, maxcol);

        //info ("%d %d", ul->range_shift->brrow, - ul->range_shift->tlrow + 1, ul->range_shift->brcol - ul->range_shift->tlcol + 1);
        shift_range(ul->range_shift->delta_rows, ul->range_shift->delta_cols,
            ul->range_shift->tlrow, ul->range_shift->tlcol, ul->range_shift->brrow, ul->range_shift->brcol);
    }

    // Borro los ent de removed
    struct ent * i = ul->removed;
    while (i != NULL) {
        struct ent * pp = *ATBL(tbl, i->row, i->col);
        clearent(pp);
        cleanent(pp);
        i = i->next;
    }

    // Cambio foco
    //if (ul->p_sig != NULL && ul->p_sig->removed != NULL) {
    //    currow = ul->p_sig->removed->row;
    //    curcol = ul->p_sig->removed->col;
    //}

    // Agrego los ent de added
    struct ent * j = ul->added;
    while (j != NULL) {
        struct ent * e_now = lookat(j->row, j->col);
        (void) copyent(e_now, j, 0, 0, 0, 0, j->row, j->col, 0);
        j = j->next;
    }

    currow = ori_currow;
    curcol = ori_curcol;

    info("Change: %d of %d", undo_list_pos + 1, len_undo_list());
    undo_list_pos++;

    return;
}